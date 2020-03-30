#include "zookeeperclient.h"

#include <string>
//#define NDEBUG  1
#include <assert.h>
#include <cstring>
#include <functional>
#include <stdarg.h>

#include <zookeeper/zookeeper.h>
#include "zookeeperclientutils.h"

using namespace std;

void logzk(const char* fmt,...)
{
    va_list va;
    char sztime[64]={0};
    char buf[1024]={0};
    va_start(va,fmt);
    vsnprintf(buf,sizeof(buf),fmt,va);
    va_end(va);

    time_t tt;
    time( &tt);
    struct tm* tm = localtime(&tt);
    snprintf(sztime, sizeof(sztime), "%4d%02d%02d %02d:%02d:%02d ",
        tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
        tm->tm_hour, tm->tm_min, tm->tm_sec);
    string content = sztime;
    content += buf;
    content += "\n";
    // try to write the special log content.
    FILE* fp = fopen("zookeeper.log","a");
    fputs(content.c_str(),fp);
    fclose(fp);
}

ZookeeperClient::ZookeeperClient()
{
//  cout<<"{ "<<__FUNCTION__<<" }"<<endl;
    logzk("ZookeeperClient::ZookeeperClient()");

    ZooLogLevel log_level = ZOO_LOG_LEVEL_ERROR;//ZOO_LOG_LEVEL_INFO;
    zoo_set_debug_level(log_level);
    m_session_state = ZOO_CONNECTING_STATE;
}

ZookeeperClient::ZookeeperClient(const string &server, int timeout, bool debug)
{
//    cout<<"{ "<<__FUNCTION__<<" }"<<endl;
    logzk("ZookeeperClient::ZookeeperClient(%s,%d,%d)",server.c_str(),timeout,debug);

    ZooLogLevel log_level = debug ? ZOO_LOG_LEVEL_DEBUG : ZOO_LOG_LEVEL_ERROR;
    zoo_set_debug_level(log_level);

    m_server = server;
    m_session_timeout = timeout;
    m_session_state = ZOO_CONNECTING_STATE;
}

ZookeeperClient::~ZookeeperClient()
{
//    cout<<"{ "<<__FUNCTION__<<" }"<<endl;
    logzk("ZookeeperClient::~ZookeeperClient()");

    closeServer();
}

void ZookeeperClient::setServerIP(const string &server, int timeout, bool debug )
{
    m_server = server;
    m_session_timeout = timeout;
    m_session_state = ZOO_CONNECTING_STATE;
}

void ZookeeperClient::SetConnectedWatcherHandler(ConnectedWatcherHandler handler)
{
    m_connectedWatcherHandler = handler;
}

void ZookeeperClient::connectingSessionWatcher(int type, int state,
                                               const shared_ptr<ZookeeperClient> &zkClientPtr, void *context)
{
//  cout<<"{ "<<__FUNCTION__<<" }"<<endl;
    logzk("ZookeeperClient::connectingSessionWatcher()");

    if(ZOO_SESSION_EVENT == type)
    {
        unique_lock<mutex> lock(m_connect_mutex);
        m_session_state = state;
        // 连接建立，记录协商后的会话过期时间，唤醒connectServer函数(只有第一次有实际作用)

        if(ZOO_CONNECTED_STATE == state)
        {
            // 连接建立
            m_session_timeout = zoo_recv_timeout(m_zkHandle);
            printf("session_timeout=%ld\n", m_session_timeout);
            if(m_connectedWatcherHandler)
                m_connectedWatcherHandler();

        }else if(ZOO_EXPIRED_SESSION_STATE == state)  // 会话过期，唤醒init函数
        {
//            closeServer();
            cout<<"*******************{ "<<__FUNCTION__<<" }*******************"<<endl;
            cout<<"*******************Zookeeper connectingSessionWatcher ZOO_EXPIRED_SESSION_STATE*******************"<<endl;
            cout<<"*******************Zookeeper ReconnectServer *******************"<<endl<<endl<<endl<<endl<<endl<<endl;
            reConnectServer();
        }else if(ZOO_AUTH_FAILED_STATE == state) // 认证失败
        {
            closeServer();
        }
//        m_connect_cond.notify_all();
    }
}

bool ZookeeperClient::connectServer()
{
//  cout<<"{ "<<__FUNCTION__<<" }"<<endl;
    logzk("ZookeeperClient::connectServer()");

    const clientid_t *clientid = zoo_client_id(m_zkHandle);
    ZkWatcherOperateContext *context = new ZkWatcherOperateContext(m_server, (void*)"connect server", shared_from_this());
    context->m_sessionWatcherHandler = std::bind(&ZookeeperClient::connectingSessionWatcher, this,
                                                 std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4);

    m_zkHandle = zookeeper_init(m_server.c_str(), sessionWatcher, m_session_timeout, 0, context, 0);
    if (m_zkHandle)
        return true;
    else
        return false;

//    unique_lock<mutex> lock(m_connect_mutex);
//    while(m_session_state != ZOO_CONNECTED_STATE && m_session_state != ZOO_EXPIRED_SESSION_STATE)
//    {
//        m_connect_cond.wait(lock, [this]{
//            return (m_session_state == ZOO_CONNECTED_STATE) || (m_session_state == ZOO_EXPIRED_SESSION_STATE); });
//    }
//    // 会话过期，fatal级错误 退出
//    return m_session_state == ZOO_CONNECTED_STATE;
}

void ZookeeperClient::closeServer()
{
//  cout<<"{ "<<__FUNCTION__<<" }"<<endl;
    logzk("ZookeeperClient::closeServer()");

    if(m_zkHandle)
    {
        zookeeper_close(m_zkHandle);
        m_zkHandle = nullptr;
    }
}

bool ZookeeperClient::reConnectServer()
{
//  cout<<"{ "<<__FUNCTION__<<" }"<<endl;
    logzk("ZookeeperClient::reConnectServer()");

    closeServer();
    return connectServer();
}

int ZookeeperClient::getNodeValue(const string &path,string &value, int &version)
{
    char buffer[MAX_BUFF_LEN];
    int length = sizeof(buffer);
    bzero(buffer, length);

    struct Stat stat;
    int result = zoo_get(m_zkHandle, path.c_str(), 0, buffer, &length, &stat);
    if(result == ZOK)
    {
        value.assign(buffer,length);
        version = stat.version;
    }
    return result;
}

int ZookeeperClient::getNodeValueWithWatcher(const string &path,string &value, int &version,
        GetNodeWatcherHandler getNodeWatcherHandler, void *context)
{
    char buffer[MAX_BUFF_LEN];
    int length = sizeof(buffer);
    bzero(buffer, length);

    Stat stat;
    ZkWatcherOperateContext *watchContext = new ZkWatcherOperateContext(path, context, shared_from_this());
    watchContext->m_getNodeWatcherHandler = getNodeWatcherHandler;

    int result = zoo_wget(m_zkHandle, path.c_str(), getNodeValueWatcher, watchContext, buffer, &length, &stat);
    if(result == ZOK)
    {
        value.assign(buffer, length);
        version = stat.version;
    }
    return result;
}

int ZookeeperClient::getNodeACL(const string &path, vector<ACL> &vectorACL)
{
    ACL_vector aclVector;
    int result  = zoo_get_acl(m_zkHandle, path.c_str(), &aclVector, 0);
    if (result == ZOK)
    {
        for(int i = 0; i < aclVector.count; i++)
            vectorACL.push_back(aclVector.data[i]);
    }
    deallocate_ACL_vector(&aclVector);
    return result;
}

int ZookeeperClient::setNodeValue(const string &path,
        const string &value, int version, struct Stat * stat)
{
    int result = zoo_set2(m_zkHandle, path.c_str(), value.c_str(), value.length(), version, stat);
    return result;
}

int ZookeeperClient::existsNode(const string &path, struct Stat *stat)
{
    if (!m_zkHandle) {
        logzk("ZookeeperClient::existsNode, but zkhandle=NULL,reconnect...");
        reConnectServer();
    }

    // connect value.
    int result = -1;
    if (m_zkHandle) {
        result = zoo_exists(m_zkHandle, path.c_str(), 0, stat);
    }

//Cleanup:
    return result;
}

int ZookeeperClient::existsNodeWithWatch(const string &path, ExistsNodeWatcherHandler existsNodeWatcherHandler, void *context)
{
    ZkWatcherOperateContext * watchContext = new ZkWatcherOperateContext(path, context, shared_from_this());
    watchContext->m_existsNodeWatcherHandler = existsNodeWatcherHandler;

    int result = zoo_wexists(m_zkHandle, path.c_str(), existsNodeWatcher, watchContext, 0);
    return result;
}

int ZookeeperClient::createNode(const string &path, const string &value, bool isTemp, bool isSequence)
{
    int flag = 0;
    if(isTemp)
        flag |= ZOO_EPHEMERAL;//temporary node

    if (isSequence)
        flag |=ZOO_SEQUENCE;

    int result = zoo_create(m_zkHandle, path.c_str(), value.c_str(), value.length(), &ZOO_OPEN_ACL_UNSAFE, flag, 0, 0);
    return result;
}

int ZookeeperClient::deleteNode(const string &path, int version)
{
    int result = zoo_delete(m_zkHandle, path.c_str(), version);
    return result;//== ZOK || result == ZNONODE;删除成功或者结点不存在，都认为是删除成功
}

int ZookeeperClient::getConnectStatus()
{
    return m_session_state;
}

//void ZookeeperClient::setConectStatus(bool connectStatus)
//{
//    std::lock_guard<std::mutex> lockGuard(mutex_);
//    connectStatus_ = connectStatus;
//}

int ZookeeperClient::getClildren(const string &path, vector<string> &childrenVec,
        GetChildrenWatcherHandler getChildrenNodeWatcherHandler, void *context)
{
    ZkWatcherOperateContext *watchContext = new ZkWatcherOperateContext(path, context, shared_from_this());
    watchContext->m_getChildrenWatcherHandler = getChildrenNodeWatcherHandler;
    /**
     * zoo_wget_children2将stat传0，会coredump(会自动传回stat,但是并没有事先判断stat是否为空)
     * int result = zoo_wget_children2(zkHandle_,path.data(),watcher,context,&stringVec,0);
     */
    String_vector stringVec;
    int result = zoo_wget_children(m_zkHandle, path.c_str(), getChildrenWatcher, watchContext, &stringVec);
    if (result == ZOK)
    {
        ZookeeperClientUtils::transStringVector2VectorString(stringVec, childrenVec);
        deallocate_String_vector(&stringVec);
    }
    return result;
}

int ZookeeperClient::getClildrenNoWatch(const string &path, vector<string> &childrenVec)
{
    String_vector stringVec;
    int result = zoo_get_children(m_zkHandle, path.c_str(), false, &stringVec);
    if (result == ZOK)
    {
        ZookeeperClientUtils::transStringVector2VectorString(stringVec, childrenVec);
        deallocate_String_vector(&stringVec);
    }
    return result;
}

/*********************************************************************************************************/
//各种默认的监视回调函数实现版本
void ZookeeperClient::sessionWatcher(zhandle_t *zh, int type, int state, const char *path, void *watcherCtx)
{
//    auto logger = spdlog::get(getGlobalLogName());
//    logger->info("[sessionWatcher] type:{} ,state:{}", watcherEventType2String(type),state2String(state));

//  cout<<"[sessionWatcher] type:{"<<ZookeeperClientUtils::watcherEventType2String(type)<<"} ,state:{"<<ZookeeperClientUtils::state2String(state)<<"}";

    string strtype = ZookeeperClientUtils::watcherEventType2String(type);
    logzk("ZookeeperClient::sessionWatcher(zh=%x,type=%s,state=%d)",zh,strtype.c_str(),state);

    ZkWatcherOperateContext *context = (ZkWatcherOperateContext *)watcherCtx;
    if (context && context->m_sessionWatcherHandler && context->m_zkClientPtr)
    {
        context->m_sessionWatcherHandler(type, state, context->m_zkClientPtr, context->m_context);
    }
}

void ZookeeperClient::existsNodeWatcher(zhandle_t *zh, int type, int state, const char *path, void *watcherCtx)
{
//    auto logger = spdlog::get(getGlobalLogName());
//    logger->info("[existsNodeWatcher] type:{} ,state:{}", watcherEventType2String(type),state2String(state));

    if(CONNECTED_STATE_DEF == state)
    {
        ZkWatcherOperateContext *context = (ZkWatcherOperateContext *)watcherCtx;
        if (context && context->m_existsNodeWatcherHandler && context->m_zkClientPtr)
        {
            context->m_existsNodeWatcherHandler(type, state, context->m_zkClientPtr, path, context->m_context);
        }
        destroyPointer(context);
    }
}

void ZookeeperClient::getNodeValueWatcher(zhandle_t *zh, int type, int state, const char *path, void *watcherCtx)
{
//    auto logger = spdlog::get(getGlobalLogName());
//    logger->info("[getNodeValueWatcher] type:{} ,state:{}", watcherEventType2String(type),state2String(state));
    if(CONNECTED_STATE_DEF == state)
    {
        ZkWatcherOperateContext *context = (ZkWatcherOperateContext *)watcherCtx;
        if (context && context->m_getNodeWatcherHandler && context->m_zkClientPtr)
        {
            context->m_getNodeWatcherHandler(type, state, context->m_zkClientPtr, path, context->m_context);
        }
        destroyPointer(context);
    }
}

void ZookeeperClient::getChildrenWatcher(zhandle_t *zh, int type, int state, const char *path, void *watcherCtx)
{
//    auto logger = spdlog::get(getGlobalLogName());
//    logger->info("[getChildrenWatcher] type:{} ,state:{}", watcherEventType2String(type),state2String(state));

    if(CONNECTED_STATE_DEF == state)
    {
        ZkWatcherOperateContext *context = (ZkWatcherOperateContext *)watcherCtx;
        if (context && context->m_getChildrenWatcherHandler && context->m_zkClientPtr)
        {
            context->m_getChildrenWatcherHandler(type, state, context->m_zkClientPtr, path, context->m_context);
        }
        destroyPointer(context);
    }
}

/*********************************************************************************************************/
//异步API对应的处理函数
void ZookeeperClient::asyncCreateNodeHandler(int rc, const char *value, const void *data)
{
//	auto logger = spdlog::get(getGlobalLogName());
//	logger->info("asyncCreateNodeHandler");
    ZkAsyncCompletionContext *completionContext = (ZkAsyncCompletionContext *)data;
    if(completionContext->m_stringCompletionHandler)
    {
        completionContext->m_stringCompletionHandler(rc, value, completionContext);
    }
    destroyPointer(completionContext);
}

void ZookeeperClient::asyncDeleteNodeHandler(int rc, const void *data)
{
//	auto logger = spdlog::get(getGlobalLogName());
//	logger->info("asyncDeleteNodeHandler");
    ZkAsyncCompletionContext *completionContext = (ZkAsyncCompletionContext *) data;
    if(completionContext->m_voidCompletionHandler)
    {
        completionContext->m_voidCompletionHandler(rc, completionContext);
    }
    destroyPointer(completionContext);
}

void ZookeeperClient::asyncGetNodeHandler(int rc, const char *value, int value_len, const struct Stat *stat, const void *data)
{
//	auto logger = spdlog::get(getGlobalLogName());
//	logger->info("asyncGetNodeHandler");
    ZkAsyncCompletionContext * completionContext = (ZkAsyncCompletionContext *)data;
    if (completionContext->m_dataCompletionHandler)
    {
        completionContext->m_dataCompletionHandler(rc, value, value_len, stat, completionContext);
    }
    destroyPointer(completionContext);
}

void ZookeeperClient::asyncSetNodeHandler(int rc, const struct Stat *stat, const void *data)
{
//	auto logger = spdlog::get(getGlobalLogName());
//	logger->info("asyncSetNodeHandler");
    ZkAsyncCompletionContext *completionContext = (ZkAsyncCompletionContext *)data;
    if (completionContext->m_statCompletionHandler)
    {
        completionContext->m_statCompletionHandler(rc, stat, completionContext);
    }
    destroyPointer(completionContext);
}

void ZookeeperClient::asyncGetChildrenHandler(int rc, const struct String_vector *strings, const void *data)
{
//	auto logger = spdlog::get(getGlobalLogName());
//	logger->info("asyncGetChildrenHandler");
    ZkAsyncCompletionContext *completionContext = (ZkAsyncCompletionContext *)data;
    if (completionContext->m_stringsCompletionHandler)
    {
        completionContext->m_stringsCompletionHandler(rc, strings, completionContext);
    }
    destroyPointer(completionContext);
}

void ZookeeperClient::asyncGetChildrenHandler2(int rc, const struct String_vector *strings,
        const struct Stat *stat,const void *data)
{
//	auto logger = spdlog::get(getGlobalLogName());
//	logger->info("asyncGetChildrenHandler2");
    ZkAsyncCompletionContext *completionContext = (ZkAsyncCompletionContext *)data;
    if (completionContext->m_stringsStatCompletionHandler)
    {
        completionContext->m_stringsStatCompletionHandler(rc, strings, stat, completionContext);
    }
    destroyPointer(completionContext);
}

void ZookeeperClient::asyncGetNodeACLHandler(int rc, struct ACL_vector *acl, struct Stat *stat, const void *data)
{
//	auto logger = spdlog::get(getGlobalLogName());
//	logger->info("asyncGetAclHandler");
    ZkAsyncCompletionContext *completionContext = (ZkAsyncCompletionContext *)data;
    if (completionContext->m_aclCompletionHandler)
    {
        completionContext->m_aclCompletionHandler(rc, acl, stat, completionContext);
    }
    destroyPointer(completionContext);
}

void ZookeeperClient::asyncSetNodeACLHandler(int rc, const void *data)
{
//	auto logger = spdlog::get(getGlobalLogName());
//	logger->info("asyncSetNodeACLHandler");
    ZkAsyncCompletionContext *completionContext = (ZkAsyncCompletionContext *)data;
    if (completionContext->m_voidCompletionHandler)
    {
        completionContext->m_voidCompletionHandler(rc,completionContext);
    }
    destroyPointer(completionContext);
}

/*********************************************************************************************************/
//异步API函数封装
int ZookeeperClient::asyncCreateNode(const string &path,const string &value,
        StringCompletionHandler handler, void * context, bool isTemp ,bool isSequence)
{
    int flag = 0;
    if(isTemp)
        flag |= ZOO_EPHEMERAL;//temporary node

    if (isSequence)
        flag |=ZOO_SEQUENCE;

    ZkAsyncCompletionContext *completionContext = new ZkAsyncCompletionContext(path, context, shared_from_this());
    completionContext->m_stringCompletionHandler = handler;

    int result = zoo_acreate(m_zkHandle, path.c_str(), value.c_str(), value.length(),
        &ZOO_OPEN_ACL_UNSAFE, flag, asyncCreateNodeHandler, completionContext);

    return result;
}

int ZookeeperClient::asyncDeleteNode(const string &path,int version,
        VoidCompletionHandler handler, void * context)
{
//    logger_->info("asyncDeleteNode path[{}]",path);
    ZkAsyncCompletionContext *completionContext = new ZkAsyncCompletionContext(path, context, shared_from_this());
    completionContext->m_voidCompletionHandler = handler;

    int result = zoo_adelete(m_zkHandle, path.c_str(), version, asyncDeleteNodeHandler, completionContext);
    return result;
}

int ZookeeperClient::asyncGetNodeValue(const string &path, DataCompletionHandler handler, void *context )
{
//    logger_->info("asyncGetNodeValueWithWatcher path[{}]",path);
    ZkAsyncCompletionContext *completionContext = new ZkAsyncCompletionContext(path, context, shared_from_this());
    completionContext->m_dataCompletionHandler = handler;
    int result = zoo_aget(m_zkHandle, path.c_str(), 0, asyncGetNodeHandler, completionContext);
    return result;
}

int ZookeeperClient::asyncGetNodeValueWithWatcher(const string &path,
        GetNodeWatcherHandler watchHandler, void *watchContext, DataCompletionHandler completionHandler, void *completionContext)
{
//    logger_->info("asyncGetNodeValueWithWatcher path[{}],value[{}]",path,value);
    ZkWatcherOperateContext *watcherOperateContext = new ZkWatcherOperateContext(path, watchContext, shared_from_this());
    watcherOperateContext->m_getNodeWatcherHandler = watchHandler;

    ZkAsyncCompletionContext *asyncCompletionContext = new ZkAsyncCompletionContext(path, completionContext, shared_from_this());
    asyncCompletionContext->m_dataCompletionHandler = completionHandler;

    int result = zoo_awget(m_zkHandle, path.c_str(), getNodeValueWatcher, watcherOperateContext,
                           asyncGetNodeHandler, asyncCompletionContext);
    return result;
}

int ZookeeperClient::asyncSetNodeValue(const string &path, const string &value, int version,
        StatCompletionHandler handler, void *context)
{
//    logger_->info("asyncSetNodeValue path[{}],value[{}]",path,value);
    ZkAsyncCompletionContext *completionContext = new ZkAsyncCompletionContext(path, context, shared_from_this());
    completionContext->m_statCompletionHandler = handler;

    int result = zoo_aset(m_zkHandle, path.c_str(), value.c_str(), value.length(), version,
                          asyncSetNodeHandler, completionContext);
    return result;
}

int ZookeeperClient::asyncGetChildren(const string &path, StringsCompletionHandler handler, void *context)
{
//    logger_->info("asyncGetChildren path[{}]",path);
    ZkAsyncCompletionContext *completionContext = new ZkAsyncCompletionContext(path, context, shared_from_this());
    completionContext->m_stringsCompletionHandler = handler;
    int result = zoo_aget_children(m_zkHandle, path.c_str(), 0, asyncGetChildrenHandler, completionContext);
    return result;
}

int ZookeeperClient::asyncGetChildrenWithWatcher(const string &path, GetChildrenWatcherHandler watchHandler,
        void *watchContext, StringsCompletionHandler completionHandler, void *context)
{
//    logger_->info("asyncGetChildrenWithWatcher path[{}]",path);
    ZkWatcherOperateContext *watcherContext = new ZkWatcherOperateContext(path, watchContext, shared_from_this());
    watcherContext->m_getChildrenWatcherHandler = watchHandler;

    ZkAsyncCompletionContext *completionContext = new ZkAsyncCompletionContext(path, context, shared_from_this());
    completionContext->m_stringsCompletionHandler = completionHandler;

    int result = zoo_awget_children(m_zkHandle, path.c_str(), getChildrenWatcher, watcherContext,
                                    asyncGetChildrenHandler, completionContext);
    return result;
}

int ZookeeperClient::asyncGetChildren2(const string &path, StringsStatCompletionHandler handler, void *context)
{
//    logger_->info("asyncGetChildren2 path[{}]",path);
    ZkAsyncCompletionContext *completionContext = new ZkAsyncCompletionContext(path, context, shared_from_this());
    completionContext->m_stringsStatCompletionHandler = handler;

    int result = zoo_aget_children2(m_zkHandle, path.c_str(), 0, asyncGetChildrenHandler2, completionContext);
    return result;
}

int ZookeeperClient::asyncGetChildren2WithWatcher(const string &path, GetChildrenWatcherHandler watchHandler,
        void *watchContext, StringsStatCompletionHandler completionHandler, void *completionContext)
{
//    logger_->info("asyncGetChildren2WithWatcher path[{}]",path);
    ZkWatcherOperateContext *watcherContext = new ZkWatcherOperateContext(path, watchContext, shared_from_this());
    watcherContext->m_getChildrenWatcherHandler = watchHandler;

    ZkAsyncCompletionContext *asyncCompletionContext = new ZkAsyncCompletionContext(path, completionContext, shared_from_this());
    asyncCompletionContext->m_stringsStatCompletionHandler = completionHandler;

    int result = zoo_awget_children2(m_zkHandle, path.c_str(), getChildrenWatcher, watcherContext,
                                     asyncGetChildrenHandler2, asyncCompletionContext);
    return result;
}

int ZookeeperClient::asyncGetNodeACL(const string &path, AclCompletionHandler handler, void *context )
{
//    logger_->info("asyncGetNodeACL path[{}]",path);
    ZkAsyncCompletionContext *completionContext = new ZkAsyncCompletionContext(path, context, shared_from_this());
    completionContext->m_aclCompletionHandler = handler;

    int result  = zoo_aget_acl(m_zkHandle, path.c_str(), asyncGetNodeACLHandler, completionContext);
    return result;
}

int ZookeeperClient::asyncSetNodeACL(const string &path, ACL_vector *aclVector, int version,
        VoidCompletionHandler handler,void *context)
{
//    logger_->info("asyncSetNodeACL path[{}]",path);
    ZkAsyncCompletionContext *completionContext = new ZkAsyncCompletionContext(path, context, shared_from_this());
    completionContext->m_voidCompletionHandler = handler;

    int result  = zoo_aset_acl(m_zkHandle, path.c_str(), version, aclVector, asyncSetNodeACLHandler, completionContext);
    return result;
}