#include "RedisClient.h"

#include <thread>


#ifdef __cplusplus
extern "C" {
#endif

#include "hiredis.h"
#include "hircluster.h"

#ifdef __cplusplus
}
#endif
#include "json/json.h"

#include <muduo/base/Logging.h>
#include <boost/algorithm/string.hpp>
#include <algorithm>
//#include "../crypto/crypto.h"

const string passWord = "AliceLandy@20181024";


string string_replace(string strbase, string src, string dst)
{
    string::size_type pos = 0;
    string::size_type srclen = src.size();
    string::size_type dstlen = dst.size();
    pos = strbase.find(src,pos);
    while (pos != string::npos)
    {
        strbase.replace(pos, srclen, dst);
        pos = strbase.find(src,(pos+dstlen));
    }
//Cleanup:
    return strbase;
}

RedisClient::RedisClient()
{
    m_redisClientContext = NULL;
}

RedisClient::~RedisClient()
{
#if USE_REDIS_CLUSTER
    if(m_redisClientContext)
        redisClusterFree(m_redisClientContext);
#else
    if(m_redisClientContext)
        redisFree(m_redisClientContext);
#endif
}

#if USE_REDIS_CLUSTER
bool RedisClient::initRedisCluster(string ip)
{
    m_redisClientContext = redisClusterContextInit();
    redisClusterSetOptionAddNodes(m_redisClientContext, ip.c_str());
    redisClusterConnect2(m_redisClientContext);
    if (m_redisClientContext != NULL && m_redisClientContext->err)
    {
        printf("Error: %s\n\n\n\n", m_redisClientContext->errstr);
        // handle error
        return false;
    }else
        return true;
}

#elif 0

bool RedisClient::initRedisCluster(string ip)
{
//    struct timeval timeout = {1, 500000 };
//    m_redisClientContext = redisConnectWithTimeout(ip.c_str(), 6379, timeout);
    m_redisClientContext = redisConnect(ip.c_str(), 6379);

    if(m_redisClientContext->err)
        printf("redis %s\n\n\n\n", m_redisClientContext->errstr);
    return !m_redisClientContext->err;
}

#else
bool RedisClient::BlackListHget(string key,  string keyson, redis::RedisValue& values,map<string,int16_t> &usermap)
{
    bool OK = false;
    values.reset();
    usermap.clear();
    while(true)
    {
        string cmd = "HMGET " + key+" "+keyson;

        // try to build the special command to read redis map value the the reply content.
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, cmd.c_str());
        if(reply)
        {
            OK = true;

            vector<string> vec;
            if(!reply->element||!reply->element[0]->str)
            {
                OK = false;
                break;
            }
            string covers=reply->element[0]->str;
            covers.erase(std::remove(covers.begin(), covers.end(), '"'), covers.end());
            boost::algorithm::split( vec,covers, boost::is_any_of( "|" ));
            for(int i=0;i<(int)vec.size();i++)
            {
                vector<string> vec1;
                boost::algorithm::split( vec1,vec[i] , boost::is_any_of( "," ));
                if(vec1.size()==2)
                {
                    usermap.insert(pair<string,int16_t>(vec1[0],stoi(vec1[1])));
                }
                vec1.clear();
            }

            freeReplyObject(reply);
            break;
        }else
            break;
    }

//Cleanup:
    return OK;
}

bool RedisClient::initRedisCluster(string ip, string password)
{
    string masterIp("127.0.0.1");
    int masterPort=6379;

    struct timeval timev;
    timev.tv_sec = 3;
    timev.tv_usec = 0;

    vector<string> vec;
    boost::algorithm::split(vec, ip, boost::is_any_of( "," ));
    if(getMasterAddr(vec, timev, masterIp, masterPort) == 0)
    {

    }else
    {
        vector<string> ipportVec;
        boost::algorithm::split(ipportVec, ip, boost::is_any_of( ":" ));
        masterIp = ipportVec[0];
        masterPort = stoi(ipportVec[1]);
    }

    m_redisClientContext = redisConnectWithTimeout(masterIp.c_str(), masterPort, timev);
    if(m_redisClientContext->err)
    {
        printf("redis %s\n\n\n\n", m_redisClientContext->errstr);
        return false;
    }else
    {
        m_ip = ip;
    }
    if(!password.empty())
        return auth(password);
    else
        return true;
}

bool RedisClient::initRedisCluster(string ip, map<string, string> &addrMap, string password)
{
    string masterIp("127.0.0.1");
    int masterPort=6379;

    struct timeval timev;
    timev.tv_sec = 3;
    timev.tv_usec = 0;

    vector<string> vec;
    boost::algorithm::split(vec, ip, boost::is_any_of( "," ));
    if(getMasterAddr(vec, timev, masterIp, masterPort) == 0)
    {
        if(addrMap.count(masterIp))
            masterIp = addrMap[masterIp];
    }else
    {
        vector<string> ipportVec;
        boost::algorithm::split(ipportVec, ip, boost::is_any_of( ":" ));
        masterIp = ipportVec[0];
        masterPort = stoi(ipportVec[1]);
    }

    m_redisClientContext = redisConnectWithTimeout(masterIp.c_str(), masterPort, timev);
    if(m_redisClientContext->err)
    {
        printf("redis %s\n\n\n\n", m_redisClientContext->errstr);
        return false;
    }else
    {
        m_ip = ip;
    }
    if(!password.empty())
        return auth(password);
    else
        return true;
}

#endif

//通过Sentinel获得master的ip和port
int RedisClient::getMasterAddr(const vector<string> &addVec, struct timeval timeOut,string& masterIp, int& masterPort)
{
    //建立和sentinel的连接
    string strPort;
    redisContext* context = NULL;
    redisReply *reply = NULL;
    for(int i = 0; i < addVec.size(); ++i)
    {
        vector<string> vec;
        boost::algorithm::split(vec, addVec[i], boost::is_any_of( ":" ));

        printf("i[%d], ip[%s], port[%d]", i, vec[0].c_str(), stoi(vec[1]));
        context = redisConnectWithTimeout(vec[0].c_str(), stoi(vec[1]), timeOut);
//        context = redisConnect(vec[0].c_str(), stoi(vec[1]));
        if (context == NULL || context->err)
        {
            printf("Connection error: can't allocate redis context,will find next");
            redisFree(context);//断开连接并释放redisContext空间
            continue;
        }

        //获取master的ip和port
        reply = static_cast<redisReply*> ( redisCommand(context,"SENTINEL get-master-addr-by-name mymaster") );
        if(reply->type != REDIS_REPLY_ARRAY || reply -> elements != 2)
        {
            printf("use sentinel to get-master-addr-by-name failure, will find next\n");
            freeReplyObject(reply);
            continue;
        }
        masterIp = reply -> element[0] -> str;
        strPort = reply -> element[1] -> str;
        masterPort = stoi(strPort);
        break;
    }
    if(masterIp.empty() || strPort.empty())
        return -1;
    return 0;
}

bool RedisClient::auth(string pass)
{
    bool OK = false;
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "AUTH %s", pass.c_str());
        if(reply)
        {
            if(reply->str  && string(reply->str) == "OK")
                OK = true;
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}


bool RedisClient::ReConnect()
{
//    return REDIS_OK==redisReconnect(m_redisClientContext);
    return initRedisCluster(m_ip);
}

bool RedisClient::get(string key, string &value)
{
    bool OK = false;
    value = "";
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "GET %s", key.c_str());
        if(reply)
        {
            if(reply->str)
            {
                value = reply->str;
                OK = true;
            }else
                value = "";
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}

bool RedisClient::set(string key, string value, int timeout)
{
    bool OK = false;

    while(true)
    {
        redisReply *reply = NULL;
        if(timeout)
        {
            reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "SET %s %s EX %d", key.c_str(), value.c_str(), timeout);
        }else
        {
            reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "SET %s %s", key.c_str(), value.c_str());
        }

        if(reply)
        {
//            LOG_INFO << " >>> redisClient::set reply type:" << reply->type;
//            LOG_INFO << " >>> string value:" << reply->str << ",integer:" << reply->integer << "timeout:" << timeout;

            if(reply->str  && string(reply->str) == "OK")
                OK = true;
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }

    return OK;
}

bool RedisClient::del(string key)
{
    bool OK = false;
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "DEL %s", key.c_str());
        if(reply)
        {
            if(reply->integer == 1)
                OK = true;
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}

int RedisClient::TTL(string key)
{
    int ret = 0;
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "TTL %s", key.c_str());
        if(reply)
        {
            ret = reply->integer;
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return ret;
}

bool RedisClient::resetExpired(string key, int timeout)
{
    bool OK = false;
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "EXPIRE %s %d", key.c_str(), timeout);
        if(reply)
        {
            if(reply->integer == 1)
                OK = true;
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}
// 过期时间为毫秒
bool RedisClient::resetExpiredEx(string key, int timeout)
{
    bool OK = false;
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "PEXPIRE %s %d", key.c_str(), timeout);
        if(reply)
        {
            if(reply->integer == 1)
                OK = true;
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}

bool RedisClient::exists(string key)
{
    bool OK = false;
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "EXISTS %s", key.c_str());
        if(reply)
        {
            if(reply->integer == 1)
                OK = true;
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}

bool RedisClient::persist(string key)
{
    bool OK = false;
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "PERSIST %s", key.c_str());
        if(reply)
        {
            if(reply->integer == 1)
                OK = true;
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }

//Cleanup:
//  LOG_DEBUG << " >>> RedisClient::persist key:" << key << ", Ok:" << OK;
    return OK;
}

bool RedisClient::hget(string key, string field, string &value)
{
    bool OK = false;
    value = "";
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "HGET %s %s", key.c_str(), field.c_str());
        if(reply)
        {
    //      LOG_INFO << " >>> redisClient::get key:" << key << ",field:" << field << ", reply type:" << reply->type;
    //      LOG_INFO << " >>> string value:" << reply->str << ",integer:" << reply->integer;

            if(reply->str)
            {
                value = reply->str;
                OK = true;
            }else
                value = "";
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}

bool RedisClient::hset(string key, string field, string value, int timeout)
{
    bool OK = false;
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "HSET %s %s %s", key.c_str(), field.c_str(), value.c_str());
        if(reply)
        {
            if(reply->str && string(reply->str) == "OK")
                OK = true;
            freeReplyObject(reply);

            // set expired.
            if(timeout)
            {
                resetExpired(key, timeout);
            }
            break;
        }else
            ReConnect();
    }
    return OK;
}

bool RedisClient::hmget(string key, string* fields, int count, redis::RedisValue& values)
{
    bool OK = false;
    values.reset();

    while(true)
    {
        if ((!fields)||(!count))
            break;
        string cmd = "HMGET " + key;
        for (int i=0;i<count;i++)
        {
            cmd += " " + fields[i];
        }

        // try to build the special command to read redis map value the the reply content.
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, cmd.c_str());
        if(reply)
        {
            OK = true;
            for(int i = 0; i < reply->elements; ++i)
            {
                redisReply *tmpReply = reply->element[i];
                string field = fields[i];
                if(tmpReply->str)
                {
                    values[field] = tmpReply->str;
                }
//                else {
//                    values[field] = "";
//                }
            }

            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }

//Cleanup:
    return OK;
}

bool RedisClient::hmset(string key, redis::RedisValue& values, int timeout)
{
    bool OK = false;
    string cmd = "HMSET " + key;
    map<string,redis::RedisValItem> listval = values.get();
    map<string,redis::RedisValItem>::iterator iter;
    for (iter=listval.begin();iter!=listval.end();++iter)
    {
        string field = iter->first;
        redis::RedisValItem& item = iter->second;
        string value = item.asString();
        if (value.length())
        {
            value = string_replace(value," ","_");
            cmd += " " + field + " " + value;
        }
    }

    while(true)
    {
        // try to build the special command to read redis map value the the reply content.
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, cmd.c_str());
        if(reply)
        {
            OK = false;
            if(reply->str && string(reply->str) == "OK")
                OK = true;
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }

    // set expired.
    if (timeout)
    {
        resetExpired(key,timeout);
    }   else
    {
        persist(key);
    }

    return OK;
}

bool RedisClient::hmget(string key, vector<string>fields, vector<string> &values)
{
    bool OK = false;
    values.resize(fields.size());

    string cmd = "HMGET " + key;
    for(string field : fields)
    {
        cmd += " "+field;
    }

    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, cmd.c_str());
        if(reply)
        {
            OK = true;
            for(int i = 0; i < reply->elements; ++i)
            {
                redisReply *tmpReply = reply->element[i];
                if(tmpReply->str)
                    values[i] = tmpReply->str;
                else
                    values[i] = "";
            }
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}

bool RedisClient::hmset(string key, vector<string>fields, vector<string> values,int timeout)
{
    bool OK = false;
    string cmd = "HMSET " + key;
    if(fields.size() == values.size())
    {
        for(int i = 0; i < fields.size(); ++i)
        {
            if (values[i].length())
            {
                cmd += " "+fields[i] + " "+values[i];
            }
        }

        while(true)
        {
            redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, cmd.c_str());
            if(reply)
            {
                if(reply->str && string(reply->str) == "OK")
                    OK = true;
                freeReplyObject(reply);
                break;
            }else
                ReConnect();
        }
    }

    // set expired.
    if (timeout)
    {
        resetExpired(key,timeout);
    }   else
    {
        persist(key);
    }

    return OK;
}

bool RedisClient::hmset(string key, map<string,string> fields,int timeout)
{
    bool OK = false;
    string cmd = "HMSET " + key;
    map<string,string>::iterator iter;
    for (iter=fields.begin();iter!=fields.end();++iter)
    {
        string field = iter->first;
        string value = iter->second;
        if (value.length())
        {
            cmd += " " + field + " " + value;
        }
    }

    while(true)
    {
        // try to build the special command to read redis map value the the reply content.
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, cmd.c_str());
        if(reply)
        {
            OK = true;
            if(reply->str && string(reply->str) == "OK")
                OK = true;
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }

    // set expired.
    if (timeout) {
        resetExpired(key,timeout);
    }else
    {
        persist(key);
    }

    return OK;

}

bool RedisClient::hdel(string key, string field)
{
    bool OK = false;
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "HDEL %s %s", key.c_str(), field.c_str());
        if(reply)
        {
            if(reply->integer == 1)
                OK = true;
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}

bool RedisClient::exists(string key, string field)
{
    bool OK = false;
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "HEXISTS %s %s", key.c_str(), field.c_str());
        if(reply)
        {
            if(reply->integer == 1)
                OK = true;
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}
// edit by caiqing
// @long -> int64_t
bool RedisClient::hincrby(string key, string field, int64_t inc, int64_t* result)
{
    bool bSuccess = false;
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "HINCRBY %s %s %lld", key.c_str(), field.c_str(), inc);
        if(reply)
        {
            if (reply->type == REDIS_REPLY_INTEGER)
            {
                if (result)
                    *result = reply->integer;
                bSuccess = true;
            }

            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }

//Cleanup:
    return bSuccess;
}

bool RedisClient::hincrby_float(string key, string field, double inc, double* result)
{
    string tmp("");
    bool bSuccess = false;
    string strIncNum = std::to_string(inc);
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "HINCRBYFLOAT %s %s %s", key.c_str(), field.c_str(), strIncNum.c_str());
        if(reply)
        {
            if(reply->type == REDIS_REPLY_STRING)
            {
                tmp = reply->str;
                if (result)
                    *result = strtod(tmp.c_str(),NULL);
                bSuccess = true;
            }

            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }

//Cleanup:
    return bSuccess;
}

bool RedisClient::blpop(string key, string &value, int timeOut)
{
    bool OK = false;

    value = "";
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "BLPOP %s %d", key.c_str(), timeOut);
        if(reply)
        {
            if(reply->elements == 2)
            {
                redisReply *tmpKey = reply->element[0];
                if(tmpKey && tmpKey->str)
                {
                    if(!strcmp(tmpKey->str, key.c_str()))
                    {
                        redisReply *tmpValue = reply->element[1];
                        if(tmpValue && tmpValue->str)
                        {
                            value = tmpValue->str;
                            OK = true;
                        }

                    }
                }
            }

            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }

    return OK;
}

// LLen key : query queue size.
bool RedisClient::rpush(string key, string value)
{
    bool OK = false;
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "RPUSH %s %s", key.c_str(), value.c_str());
        if(reply)
        {
            if(reply->integer == 1)
                OK = true;
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}

// LLen key : query queue size.
bool RedisClient::lpush(string key, string value, long long int &len)
{
    bool OK = false;
    len = 0;
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "LPUSH %s %s", key.c_str(), value.c_str());
        if(reply)
        {
            if(reply->integer > 0){
                OK = true;
                len = reply->integer;
            }
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}

// 移出并获取列表的第一个元素
bool RedisClient::rpop(string key, string &values){
    bool OK = false;
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "RPOP %s", key.c_str());
        if(reply)
        {
            OK = true; 
            if(reply->str){
                values = std::string(reply->str);
                // LOG_ERROR << __FILE__ << " " << values << " " << __FUNCTION__;
                // printf("rpop :%s\n",reply->str);
            } 
            
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}


bool RedisClient::lrange(string key, int startIdx, int endIdx,vector<string> &values)
{
    bool OK = false;
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "LRANGE %s %d %d", key.c_str(), startIdx, endIdx);
        if(reply)
        {
            OK = true;
            for(int i = 0; i < reply->elements; ++i)
            {
                if(reply->element[i]->str&&strcmp(reply->element[i]->str,"nil"))
                {
                    if(values.size() > i)
                    {
                        values[i] = reply->element[i]->str;
                    }else{
                        values.push_back(reply->element[i]->str);
                    }

                }else{
                    break;
                }
            }
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}

bool RedisClient::ltrim(string key, int startIdx, int endIdx)
{
    bool OK = false;
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "LTRIM %s %d %d", key.c_str(), startIdx, endIdx);
        if(reply)
        {
            if(reply->str  && string(reply->str) == "OK")
                OK = true;
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}

bool RedisClient::llen(string key,int32_t &value)
{
    bool OK = false;
    value = 0;
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "LLEN %s", key.c_str());
        if(reply)
        {
            if(reply->str)
            {
                value = reply->integer;
                OK = true;
            }else
                value = 0;
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}

// add by caiqing 
// 列表移除元素
bool RedisClient::lrem(string key, int count, string value)
{
    bool OK = false;
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "LREM %s %d %s", key.c_str(), count, value.c_str());
        if(reply)
        {
            OK = reply->integer > 0; 
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}
 
// 移除列表元素
// count = 0 : 移除与 VALUE 相等的元素，数量为 COUNT 
// count > 0 : 从表头开始向表尾搜索
// count < 0 : 从表尾开始向表头搜索
// bool RedisClient::lremCmd(eRedisKey keyId, int count, string value){
//     std::string keyName = REDIS_KEY_ID +  std::to_string((int)keyId);
//     return lrem(keyName,count,value);
// }

// 列表添加元素
// bool RedisClient::lpushCmd(eRedisKey keyId,string value,long long &len){
//     std::string keyName = REDIS_KEY_ID +  std::to_string((int)keyId);
//     return lpush(keyName,value,len);
// }

// 获取列表元素
// bool RedisClient::lrangeCmd(eRedisKey keyId,vector<string> &list,int end,int start){
//    std::string keyName = REDIS_KEY_ID +  std::to_string((int)keyId);
//    bool result = lrange(keyName,start,end,list);
//    return result;
// }

// 移除列表元素
// bool RedisClient::rpopCmd(eRedisKey keyId,string &lastElement){
//    std::string keyName = REDIS_KEY_ID +  std::to_string((int)keyId);
//    bool result = rpop(keyName,lastElement);
//    return result;
// }

// 移除集合中一个或多个成员
// bool RedisClient::sremCmd(eRedisKey keyId,string value){
//     std::string keyName = REDIS_KEY_ID + to_string((int)keyId);
//     return  srem(keyName, value);
// } 

//为集合添加元素
// bool RedisClient::saddCmd(eRedisKey keyId,string value){
//     std::string keyName = REDIS_KEY_ID + to_string((int)keyId); 
//     return sadd(keyName, value);
// }

// 获取集合元素
// bool RedisClient::smembersCmd(eRedisKey keyId,vector<string> &list){
//    std::string keyName = REDIS_KEY_ID + std::to_string((int)keyId);
//    bool result = smembers(keyName,list);
//    return result;
// }
// 移除锁
// bool RedisClient::delnxCmd(eRedisKey keyId,string & lockValue){
//     std::string keyName = REDIS_KEY_ID + std::to_string((int)keyId);
//     std::string values;
//     get(keyName,values);
//     // 相同才能解锁
//     if(values == lockValue){
//         return del(keyName);
//     }
//     else
//         return false;
// }
// 设置锁(使用条件:允许锁的偶尔失效)
// int RedisClient::setnxCmd(eRedisKey keyId, string &value,int timeout){
//     std::string keyName = REDIS_KEY_ID + std::to_string((int)keyId);
//     int ret = -1,repeat = 0;
//     while( ++repeat < 1000 ){
//         redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "SETNX %s %s", keyName.c_str(), value.c_str());
//         if(reply){
//             ret = reply->integer;  
//             freeReplyObject(reply);
//             break;
//         }else
//             ReConnect();
//     }
// 
//     // 设置过期时间
//     if (ret == 1 && timeout > 0)
//         resetExpired(keyName, timeout);
// 
//     return ret;
// }
 
//创建集合
bool RedisClient::sadd(string key, string value)
{
    // LOG_ERROR << __FILE__ << " " << key << " " << value << " " << __FUNCTION__;
    bool OK = false;
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "SADD %s %s", key.c_str(), value.c_str());
        if(reply)
        {
            OK = reply->integer > 0; 
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}

//判断Key是否为集合
bool RedisClient::sismember(string key, string value)
{
    bool OK = false;
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "SISMEMBER %s %s", key.c_str(), value.c_str());
        if(reply)
        {
            OK = reply->integer > 0;
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}

bool RedisClient::srem(string key, string value)
{
    // LOG_ERROR << __FILE__ << " " << key << " " << value << " " << __FUNCTION__;

    bool OK = false;
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "SREM %s %s", key.c_str(), value.c_str());
        if(reply)
        {
            OK = reply->integer > 0;
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}

bool RedisClient::smembers(string key, vector<string> &list)
{
    bool OK = false;
    list.clear();
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "SMEMBERS %s", key.c_str());
        if(reply)
        {
            OK = true;
            for(int i = 0; i < reply->elements; ++i)
            {
                // LOG_WARN << "elements = "<< i << " "<< reply->element[i]->str;
                // 查找完
                if(strcmp(reply->element[i]->str,"nil") == 0){
                    // LOG_ERROR << "smembers 查找完 = ";
                    break;
                }

                if(reply->element[i]->str){
                    list.push_back(reply->element[i]->str);
                }
            }
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}

//=================================================
bool RedisClient::SetUserOnlineInfo(int64_t userId, uint32_t nGameId, uint32_t nRoomId)
{
    string strKeyName = /*REDIS_ONLINE_PREFIX*/"online_" + to_string(userId);

    redis::RedisValue values;
    values["nGameId"] = nGameId;
    values["nRoomId"] = nRoomId;
    return hmset(strKeyName, values, 60 * 3 /*MAX_USER_ONLINE_INFO_IDLE_TIME*/);
}

bool RedisClient::GetUserOnlineInfo(int64_t userId, uint32_t &nGameId, uint32_t &nRoomId)
{
    bool bSuccess = false;
    redis::RedisValue redisValue;
    string strKeyName = /*REDIS_ONLINE_PREFIX*/"online_" + to_string(userId);
    string fields[]   = {"nGameId", "nRoomId"};
    bool bExist = hmget(strKeyName, fields, CountArray(fields), redisValue);
    if  ((bExist) && (!redisValue.empty()))
    {
        nGameId     = redisValue["nGameId"].asInt();
        nRoomId     = redisValue["nRoomId"].asInt();
        bSuccess = true;
    }
    return bSuccess;
}

bool RedisClient::SetUserOnlineInfoIP(int64_t userId, string ip)
{
    string strKeyName = /*REDIS_ONLINE_PREFIX*/"online_" + to_string(userId);
    return hset(strKeyName, "GameServerIP", ip, 60 * 3/*MAX_USER_ONLINE_INFO_IDLE_TIME*/);
}

bool RedisClient::GetUserOnlineInfoIP(int64_t userId, string &ip)
{
    string strKeyName = /*REDIS_ONLINE_PREFIX*/"online_" + to_string(userId);
    return hget(strKeyName, "GameServerIP", ip);
}

bool RedisClient::ResetExpiredUserOnlineInfo(int64_t userId,int timeout)
{
    string strKeyName = /*REDIS_ONLINE_PREFIX*/"online_" + to_string(userId);
    return resetExpired(strKeyName,timeout);
}

bool RedisClient::ExistsUserOnlineInfo(int64_t userId)
{
    string strKeyName = /*REDIS_ONLINE_PREFIX*/"online_" + to_string(userId);
    return exists(strKeyName);
}

bool RedisClient::DelUserOnlineInfo(int64_t userId)
{
    string strKeyName = /*REDIS_ONLINE_PREFIX*/"online_" + to_string(userId);
    return del(strKeyName);
}

int RedisClient::TTLUserOnlineInfo(int64_t userId)
{
    string strKeyName = /*REDIS_ONLINE_PREFIX*/"online_" + to_string(userId);
    return TTL(strKeyName);
}

bool RedisClient::GetGameServerplayerNum(vector<string> &serverValues,uint64_t &nTotalCount)
{
    bool OK = false;
    string cmd = "MGET " ;
    for(string &field : serverValues)
    {
        cmd += " " + field;
    }

    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, cmd.c_str());
        if(reply)
        {
            OK = true;
            for(int i = 0; i < reply->elements; ++i)
            {
                if(reply->element[i]->str&&strcmp(reply->element[i]->str,"nil"))
                {
                    vector<string> vec{"0","0"};
                    boost::algorithm::split(vec, reply->element[i]->str, boost::is_any_of( "+" ));
                    nTotalCount+=(stod(vec[0])+stod(vec[1]));
//                      nTotalCount+=(stod(reply->element[i]->str));
                }
            }
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}

bool RedisClient::GetGameRoomplayerNum(vector<string> &serverValues, map<string, uint64_t> &mapPlayerNum)
{
    bool OK = false;
    string cmd = "MGET " ;
    for(string &field : serverValues)
    {
        cmd += " " + field;
    }

    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, cmd.c_str());
        if(reply)
        {
            OK = true;
            for(int i = 0; i < reply->elements; ++i)
            {
                if(reply->element[i]->str&&strcmp(reply->element[i]->str,"nil"))
                {
                    vector<string> vec{"0","0"};
                    boost::algorithm::split(vec, reply->element[i]->str, boost::is_any_of( "+" ));
                    mapPlayerNum.emplace(serverValues[i],stod(vec[0]));
                }
            }
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}

bool RedisClient::GetGameAgentPlayerNum(vector<string> &keys, vector<string> &values)
{
    bool OK = false;
    string cmd = "MGET " ;
    for(string &field : keys)
    {
        cmd += " agentid:" + field;
    }

    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, cmd.c_str());
        if(reply)
        {
            OK = true;
            for(int i = 0; i < reply->elements; ++i)
            {
                if(reply->element[i]->str&&strcmp(reply->element[i]->str,"nil"))
                {
                    values.push_back(reply->element[i]->str);
                }
            }
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    return OK;
}


bool RedisClient::GetUserLoginInfo(int64_t userId, string field, string &value)
{
    bool ret = false;
    value = "";
    string key = /*REDIS_ACCOUNT_PREFIX*/"account_" + to_string(userId);
    if (hget(key, field, value))
    {
        ret = true;
    }
    return ret;
}

bool RedisClient::SetUserLoginInfo(int64_t userId, string field, const string &value)
{
    string key = /*REDIS_ACCOUNT_PREFIX*/"account_" + to_string(userId);
    return hset(key, field, value);
}

bool RedisClient::ResetExpiredUserLoginInfo(int64_t userId)
{
    string key = /*REDIS_ACCOUNT_PREFIX*/"account_" + to_string(userId);
    return resetExpired(key);
}

bool RedisClient::ExistsUserLoginInfo(int64_t userId)
{
    string key = /*REDIS_ACCOUNT_PREFIX*/"account_" + to_string(userId);
    return exists(key);
}

bool RedisClient::DeleteUserLoginInfo(int64_t userId)
{
    string key = /*REDIS_ACCOUNT_PREFIX*/"account_" + to_string(userId);
    return del(key);
}

bool RedisClient::AddToMatchedUser(int64_t userId, int64_t blockUser)
{
    string key = REDIS_USER_BLOCK+to_string(userId);
    long long int newLen = 0;
    bool result = lpush(key,to_string(blockUser),newLen);
    if(result)
    {
        if(newLen > 20)
        {
            result = ltrim(key,0,19);
        }
    }
    resetExpired(key,ONE_WEEK);
    if(blockUser != 0 )
    {
        key = REDIS_USER_BLOCK+to_string(blockUser);
        result = lpush(key,to_string(userId),newLen);
        if(result)
        {
            if(newLen > 20)
            {
                result = ltrim(key,0,19);
            }
        }
        resetExpired(key,ONE_WEEK);
    }
    return true;
}

bool RedisClient::GetMatchBlockList(int64_t userId,vector<string> &list)
{
    string key = REDIS_USER_BLOCK+to_string(userId);
    bool result = lrange(key,0,19,list);
    return result;
}

bool RedisClient::AddQuarantine(int64_t userId)
{
    string key = REDIS_QUARANTINE+to_string(userId);
    string value;
    bool res = get(key,value);
    if(res)
    {
        if(value != "2")
        {
            set(key,"1");
            resetExpired(key, ONE_WEEK);
        }
    }else
    {
        set(key,"1");
        resetExpired(key, ONE_WEEK);
    }
}

bool RedisClient::RemoveQuarantine(int64_t userId)
{
    string key = REDIS_QUARANTINE+to_string(userId);
    string value;
    bool res = get(key,value);
    if(res)
    {
        if(value != "2")
        {
            del(key);
        }
    }
}

//int RedisClient::TTLUserLoginInfo(int64_t userId)
//{
//    string key = REDIS_ACCOUNT_PREFIX + to_string(userId);
//    return TTL(key);
//}









//=================================================
//发布
void RedisClient::publish(string channel, string msg)
{
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "PUBLISH %s %s", channel.c_str(), msg.c_str());
        if(reply)
        {
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
}

//订阅
void RedisClient::subscribe(string channel)
{
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "SUBSCRIBE %s", channel.c_str());
        if(reply)
        {
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
}

void RedisClient::unsubscribe()
{
    while(true)
    {
        redisReply *reply = (redisReply *)REDIS_COMMAND(m_redisClientContext, "UNSUBSCRIBE");
        if(reply)
        {
            freeReplyObject(reply);
            break;
        }else
            ReConnect();
    }
    m_sub_func_map.clear();
}


void RedisClient::publishRechargeScoreMessage(string msg)
{
    publish("RechargeScoreMessage", msg);
}
void RedisClient::subscribeRechargeScoreMessage(function<void(string)> func)
{
    subscribe("RechargeScoreMessage");
    m_sub_func_map["RechargeScoreMessage"] = func;
}

void RedisClient::publishRechargeScoreToProxyMessage(string msg)
{
    publish("RechargeScoreToProxyMessage", msg);
}
void RedisClient::subscribeRechargeScoreToProxyMessage(function<void(string)> func)
{
    subscribe("RechargeScoreToProxyMessage");
    m_sub_func_map["RechargeScoreToProxyMessage"] = func;
}

void RedisClient::publishRechargeScoreToGameServerMessage(string msg)
{
    publish("RechargeScoreToGameServerMessage", msg);
}
void RedisClient::subscribeRechargeScoreToGameServerMessage(function<void(string)> func)
{
    subscribe("RechargeScoreToGameServerMessage");
    m_sub_func_map["RechargeScoreToGameServerMessage"] = func;
}

void RedisClient::publishExchangeScoreMessage(string msg)
{
    publish("ExchangeScoreMessage", msg);
}
void RedisClient::subscribeExchangeScoreMessage(function<void(string)> func)
{
    subscribe("ExchangeScoreMessage");
    m_sub_func_map["ExchangeScoreMessage"] = func;
}

void RedisClient::publishExchangeScoreToProxyMessage(string msg)
{
    publish("ExchangeScoreToProxyMessage", msg);
}
void RedisClient::subscribeExchangeScoreToProxyMessage(function<void(string)> func)
{
    subscribe("ExchangeScoreToProxyMessage");
    m_sub_func_map["ExchangeScoreToProxyMessage"] = func;
}

void RedisClient::publishExchangeScoreToGameServerMessage(string msg)
{
    publish("ExchangeScoreToGameServerMessage", msg);
}
void RedisClient::subscribeExchangeScoreToGameServerMessage(function<void(string)> func)
{
    subscribe("ExchangeScoreToGameServerMessage");
    m_sub_func_map["ExchangeScoreToGameServerMessage"] = func;
}


//推送公共消息
void RedisClient::pushPublishMsg(int msgId,string msg)
{
     publish("rs_public_msg_" + std::to_string((int)msgId), msg);
}
//订阅公共消息
void RedisClient::subscribePublishMsg(int msgId,function<void(string)> func)
{
    std::string msgName = "rs_public_msg_" + std::to_string((int)msgId);
    subscribe(msgName);
    m_sub_func_map[msgName] = func;
}

void RedisClient::publishUserLoginMessage(string msg)
{
    publish("UserLoginMessage", msg);
}

void RedisClient::subscribeUserLoginMessage(function<void(string)> func)
{
    subscribe("UserLoginMessage");
    m_sub_func_map["UserLoginMessage"] = func;
}

void RedisClient::publishUserKillBossMessage(string msg)
{
    publish("UserKillBossMessage", msg);
}
void RedisClient::subscribeUserKillBossMessage(function<void(string)> func)
{
    subscribe("UserKillBossMessage");
    m_sub_func_map["UserKillBossMessage"] = func;
}

void RedisClient::publishNewChatMessage(string msg)
{
    publish("NewChatMessage", msg);
}
void RedisClient::subscribeNewChatMessage(function<void(string)> func)
{
    subscribe("NewChatMessage");
    m_sub_func_map["NewChatMessage"] = func;
}

void RedisClient::publishNewMailMessage(string msg)
{
    publish("NewMailMessage", msg);
}
void RedisClient::subscribeNewMailMessage(function<void(string)> func)
{
    subscribe("NewMailMessage");
    m_sub_func_map["NewMailMessage"] = func;
}

void RedisClient::publishNoticeMessage(string msg)
{
    publish("NoticeMessage", msg);
}
void RedisClient::subscribeNoticeMessage(function<void(string)> func)
{
    subscribe("NoticeMessage");
    m_sub_func_map["NoticeMessage"] = func;
}

void RedisClient::publishStopGameServerMessage(string msg)
{
    publish("StopGameServerMessage", msg);
}
void RedisClient::subscribeStopGameServerMessage(function<void(string)> func)
{
    subscribe("StopGameServerMessage");
    m_sub_func_map["StopGameServerMessage"] = func;
}

void RedisClient::publishRefreashConfigMessage(string msg)
{
    publish("RefreashConfigMessage", msg);
}

void RedisClient::subsreibeRefreashConfigMessage(function<void (string)> func)
{
    subscribe("RefreashConfigMessage");
    m_sub_func_map["RefreashConfigMessage"] = func;
}

void RedisClient::publishOrderScoreMessage(string msg)
{
    publish("OrderScoreMessage",msg);
}

void RedisClient::subsreibeOrderScoreMessage(function<void (string)> func)
{
    subscribe("OrderScoreMessage");
    m_sub_func_map["OrderScoreMessage"] = func;
}


void RedisClient::startSubThread()
{
    m_redis_pub_sub_thread.reset(new std::thread(&RedisClient::getSubMessage, this));
}

void RedisClient::getSubMessage()
{
    while(true)
    {
        redisReply *preply = NULL;

#if USE_REDIS_CLUSTER
        redisClusterGetReply(m_redisClientContext, (void **)&preply);
#else
        redisGetReply(m_redisClientContext, (void **)&preply);
#endif
        if(preply)
        {
            string msgtype, channel, msg;
            if(preply->type == REDIS_REPLY_ARRAY)
            {
                if(preply->elements >= 3)
                {
                    // msg type
                    msgtype = preply->element[0]->str;
                    if(msgtype == "message")
                    {
                        // ID
                        channel = preply->element[1]->str;
                        // Msg Body
                        msg = preply->element[2]->str;
                        if(m_sub_func_map.count(channel))
                        {
                            function<void(string)> functor = m_sub_func_map[channel];
                            functor(msg);
                        }
                    }
                }
            }
        }else
            ReConnect();
        freeReplyObject(preply);
    }
}

bool RedisClient::PushSQL(string sql)
{
    return rpush("SQLQueue", sql);
}

bool RedisClient::POPSQL(string &sql, int timeOut)
{
    return blpop("SQLQueue", sql, timeOut);
}





////request on game server
//bool RedisClient::setUserIdGameServerInfo(int userId, string ip)
//{
//    return set("GameServer:"+to_string(userId), ip, MAX_LOGIN_IDLE_TIME);
//}

//bool RedisClient::getUserIdGameServerInfo(int userId, string &ip)
//{
//    ip = "";
//    return get("GameServer:" + to_string(userId), ip);
//}

//bool RedisClient::resetExpiredUserIdGameServerInfo(int userId)
//{
//    return resetExpired("GameServer:" + to_string(userId));
//}

//bool RedisClient::existsUserIdGameServerInfo(int userId)
//{
//    return exists("GameServer:" + to_string(userId));
//}

//bool RedisClient::delUserIdGameServerInfo(int userId)
//{
//    return del("GameServer:"+to_string(userId));
//}

//bool RedisClient::persistUserIdGameServerInfo(int userId)
//{
//    return persist("GameServer:"+to_string(userId));
//}

//int RedisClient::TTLUserIdGameServerInfo(int userId)
//{
//    string key = "GameServer:" + to_string(userId);
//    return TTL(key);
//}

//int RedisClient::getVerifyCode(string phoneNum, int type, string &verifyCode)  //0 getVerifycode ok   1 noet exists 2 error
//{
//    string key = phoneNum + "_" + to_string(type);
//    if(get(key, verifyCode) && !verifyCode.empty())
//        return 0;
//    else
//        return 1;
//}

//void RedisClient::setVerifyCode(string phoneNum, int type, string &verifyCode)
//{
//    string key = phoneNum + "_" + to_string(type);
//    set(key, verifyCode, MAX_VERIFY_CODE_LOGIN_IDLE_TIME);
//}

//bool RedisClient::existsVerifyCode(string phoneNum, int type)
//{
//    string key = phoneNum + "_" + to_string(type);
//    return exists(key);
//}

//bool RedisClient::setUserLoginInfo(int userId, string &account, string &password, string &dynamicPassword, int temp,
//                                 string &machineSerial, string &machineType, int nPlatformId, int nChannelId)
//{


//    string key = "LoginInfo_" + to_string(userId);
//    Json::Value jsonValue;
//    jsonValue["userId"] = userId;
//    jsonValue["strAccount"] = account;
//    jsonValue["strPassword"] = password;
//    jsonValue["dynamicPassword"] = dynamicPassword;
//    jsonValue["temp"] = temp;
//    jsonValue["strMachineSerial"] = machineSerial;
//    jsonValue["strMachineType"] = machineType;
//    jsonValue["nPlatformId"] = nPlatformId;
//    jsonValue["nChannelId"] = nChannelId;
//    Json::FastWriter writer;
//    string strValue = writer.write(jsonValue);
//    return set(key, strValue);
//}

//// cache the special user login info.
//bool RedisClient::setUserLoginInfo(int64_t userId, Global_UserBaseInfo& baseinfo)
//{
//    redis::RedisValue redisValue;
//    string key = REDIS_ACCOUNT_PREFIX + to_string(userId);

//    redisValue["userId"]             = to_string((int)baseinfo.nUserId);
//    redisValue["proxyId"]            = to_string((int)baseinfo.nPromoterId);
//    redisValue["bindProxyId"]        = to_string((int)baseinfo.nBindPromoterId);

//    redisValue["gem"]                = to_string((int)baseinfo.nGem);
//    redisValue["platformId"]         = to_string((int)baseinfo.nPlatformId);
//    redisValue["channelId"]          = to_string((int)baseinfo.nChannelId);
//    redisValue["ostype"]             = to_string((int)baseinfo.nOSType);
//    redisValue["gender"]             = to_string((int)baseinfo.nGender);
//    redisValue["headId"]             = to_string((int)baseinfo.nHeadId);
//    redisValue["headboxId"]          = to_string((int)baseinfo.nHeadboxId);
//    redisValue["vip"]                = to_string((int)baseinfo.nVipLevel);
//    redisValue["vip2"]               = to_string((int)baseinfo.nVipLevel);
//    redisValue["temp"]               = to_string((int)baseinfo.nTemp);
//    redisValue["manager"]            = to_string((int)baseinfo.nIsManager);
//    redisValue["superAccount"]       = to_string((int)baseinfo.nIsSuperAccount);

//    redisValue["totalRecharge"]      = to_string((double)baseinfo.nTotalRecharge);
//    redisValue["score"]              = to_string((double)baseinfo.nUserScore);
//    redisValue["bankScore"]          = to_string((double)baseinfo.nBankScore);
//    redisValue["chargeAmount"]       = to_string((double)baseinfo.nChargeAmount);
//    redisValue["loginTime"]          = to_string(baseinfo.nLoginTime);
//    redisValue["gameStartTime"]      = to_string(baseinfo.nGameStartTime);

//    redisValue["headUrl"]            = string(baseinfo.szHeadUrl);
//    redisValue["account"]            = string(baseinfo.szAccount);
//    redisValue["nickName"]           = string(baseinfo.szNickName);
////    redisValue["regIp"]              = string(baseinfo.szIp);
//    redisValue["loginIp"]            = string(baseinfo.szIp);

////    string strLocation = Landy::Crypto::BufferToHexString((unsigned char*)baseinfo.szLocation, strlen(baseinfo.szLocation));
////    redisValue["loginLocation"]           = strLocation; //string(baseinfo.szLocation);

//    redisValue["loginLocation"]      = string(baseinfo.szLocation);

//    redisValue["password"]           = string(baseinfo.szPassword);
//    redisValue["dynamicPassword"]    = string(baseinfo.szDynamicPass);
//    redisValue["bankPassword"]       = string(baseinfo.szBankPassword);

//    redisValue["mobileNum"]          = string(baseinfo.szMobileNum);
//    redisValue["regMachineType"]     = string(baseinfo.szMachineType);
//    redisValue["regMachineSerial"]   = string(baseinfo.szMachineSerial);

//    redisValue["aliPayAccount"]      = string(baseinfo.szAlipayAccount);
//    redisValue["aliPayName"]         = string(baseinfo.szAlipayName);

//    redisValue["bankCardNum"]        = string(baseinfo.szBankCardNum);
//    redisValue["bankCardName"]       = string(baseinfo.szBankCardName);

////Cleanup:
//    return hmset(key, redisValue);
//}

//bool RedisClient::getUserLoginInfo(int64_t userId, Global_UserBaseInfo& baseinfo)
//{
//    bool bSuccess = false;
//    do
//    {
//        redis::RedisValue redisValue;
//        string key = REDIS_ACCOUNT_PREFIX + to_string(userId);
//        string fields[] = {
//            "userId","proxyId","bindProxyId","gem","platformId","channelId", "ostype", "gender","headId",
//            "headboxId","vip","temp","manager","superAccount","totalRecharge","score","bankScore","chargeAmount", "loginTime", "gameStartTime",
//            "headUrl","account","nickName","loginIp","uuid","loginLocation","password","dynamicPassword",
//            "bankPassword","mobileNum","regMachineType","regMachineSerial","aliPayAccount","aliPayName","bankCardNum","bankCardName"
//        };

////        int  count = (sizeof(fields)/sizeof(fields[0]));
////        LOG_ERROR << "count = "<<count;
//        // try to get the special redis content from the set now.
//        if (exists(key) && hmget(key, fields, CountArray(fields), redisValue))
//        {
//            baseinfo.nUserId        = redisValue["userId"].asInt();
//            baseinfo.nPromoterId    = redisValue["proxyId"].asInt();
//            baseinfo.nBindPromoterId= redisValue["bindProxyId"].asInt();

//            baseinfo.nGem           = redisValue["gem"].asInt();
//            baseinfo.nPlatformId    = redisValue["platformId"].asInt();
//            baseinfo.nChannelId     = redisValue["channelId"].asInt();
//            baseinfo.nOSType        = redisValue["ostype"].asInt();
//            baseinfo.nGender        = redisValue["gender"].asInt();
//            baseinfo.nHeadId        = redisValue["headId"].asInt();
//            baseinfo.nHeadboxId     = redisValue["headboxId"].asInt();
//            baseinfo.nVipLevel      = redisValue["vip"].asInt();
//            baseinfo.nTemp          = redisValue["temp"].asInt();
//            baseinfo.nIsManager     = redisValue["manager"].asInt();
//            baseinfo.nIsSuperAccount = redisValue["superAccount"].asInt();

//            baseinfo.nTotalRecharge = redisValue["totalRecharge"].asDouble();
//            baseinfo.nUserScore     = redisValue["score"].asDouble();
//            baseinfo.nBankScore     = redisValue["bankScore"].asDouble();
//            baseinfo.nChargeAmount  = redisValue["chargeAmount"].asDouble();
//            baseinfo.nLoginTime     = redisValue["loginTime"].asInt64();
//            baseinfo.nGameStartTime = redisValue["gameStartTime"].asInt64();

//            snprintf(baseinfo.szHeadUrl,        sizeof(baseinfo.szHeadUrl),         "%s",   redisValue["headUrl"].asString().c_str());
//            snprintf(baseinfo.szAccount,        sizeof(baseinfo.szAccount),         "%s",   redisValue["account"].asString().c_str());
//            snprintf(baseinfo.szNickName,       sizeof(baseinfo.szNickName),        "%s",   redisValue["nickName"].asString().c_str());
//            snprintf(baseinfo.szIp,             sizeof(baseinfo.szIp),              "%s",   redisValue["loginIp"].asString().c_str());
//            snprintf(baseinfo.szLocation,       sizeof(baseinfo.szLocation),        "%s",   redisValue["loginLocation"].asString().c_str());

//            snprintf(baseinfo.szPassword,       sizeof(baseinfo.szPassword),        "%s",   redisValue["password"].asString().c_str());
//            snprintf(baseinfo.szDynamicPass,    sizeof(baseinfo.szDynamicPass),     "%s",   redisValue["dynamicPassword"].asString().c_str());
//            snprintf(baseinfo.szBankPassword,   sizeof(baseinfo.szBankPassword),    "%s",   redisValue["bankPassword"].asString().c_str());

//            snprintf(baseinfo.szMobileNum,      sizeof(baseinfo.szMobileNum),       "%s",   redisValue["mobileNum"].asString().c_str());
//            snprintf(baseinfo.szMachineType,    sizeof(baseinfo.szMachineType),     "%s",   redisValue["regMachineType"].asString().c_str());
//            snprintf(baseinfo.szMachineSerial,  sizeof(baseinfo.szMachineSerial),   "%s",   redisValue["regMachineSerial"].asString().c_str());

//            snprintf(baseinfo.szAlipayAccount,  sizeof(baseinfo.szAlipayAccount),   "%s",   redisValue["aliPayAccount"].asString().c_str());
//            snprintf(baseinfo.szAlipayName,     sizeof(baseinfo.szAlipayName),      "%s",   redisValue["aliPayName"].asString().c_str());

//            snprintf(baseinfo.szBankCardNum,    sizeof(baseinfo.szBankCardNum),     "%s",   redisValue["bankCardNum"].asString().c_str());
//            snprintf(baseinfo.szBankCardName,   sizeof(baseinfo.szBankCardName),    "%s",   redisValue["bankCardName"].asString().c_str());

//            bSuccess = true;
//        }
//    }while (0);

////Cleanup:
//    return bSuccess;
//}


