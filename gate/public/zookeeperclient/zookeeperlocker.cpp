#include "zookeeperlocker.h"




using namespace std;


ZookeeperLocker::ZookeeperLocker()
{

}

ZookeeperLocker::~ZookeeperLocker()
{
    zkr_lock_destroy(&m_zkr_lock_mutex);
}

void ZookeeperLocker::zookeeperLockerWatcher(int rc, void* cbdata)
{
    ZkLockerWatcherOperateContext *context = (ZkLockerWatcherOperateContext *)cbdata;
    if (context->m_lockerWatcherHandler)
    {
        context->m_lockerWatcherHandler(rc, context->m_path, context->m_context, context->m_zkLockerPtr);
    }
    if(1==rc)
    {
        delete context;
        context = nullptr;
    }
}

void ZookeeperLocker::initLocker(zhandle_t* zh, char* path, struct ACL_vector *acl, LockerWatcherHandler handler, void* context)
{
    ZkLockerWatcherOperateContext *zkLockerWatcherOperateContext =
            new ZkLockerWatcherOperateContext(zh, path, acl, context, shared_from_this());
    zkLockerWatcherOperateContext->m_lockerWatcherHandler = handler;
    zkr_lock_init_cb(&m_zkr_lock_mutex, zh, path, acl, zookeeperLockerWatcher, (void*)zkLockerWatcherOperateContext);
}

void ZookeeperLocker::destroyLocker()
{
    zkr_lock_destroy(&m_zkr_lock_mutex);
}

int ZookeeperLocker::lock()
{
    return zkr_lock_lock(&m_zkr_lock_mutex);
}

int ZookeeperLocker::unlock()
{
    return zkr_lock_unlock(&m_zkr_lock_mutex);
}

char* ZookeeperLocker::getLockPath()
{
    return zkr_lock_getpath(&m_zkr_lock_mutex);
}

int ZookeeperLocker::isOwnerLock()
{
    return zkr_lock_isowner(&m_zkr_lock_mutex);
}

char* ZookeeperLocker::getLockId()
{
    return zkr_lock_getid(&m_zkr_lock_mutex);
}
