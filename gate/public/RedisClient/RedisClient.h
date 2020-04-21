#ifndef REDISCLIENT_H
#define REDISCLIENT_H

#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <map>
#include <random>
#include <thread>

using namespace std;

//#include "gameDefine.h"

#define REDIS_POP_TIMEOUT    (1)

#define USE_REDIS_CLUSTER    (0)

#define ONE_DAY        60*60*24
#define ONE_WEEK        ONE_DAY*7

#define REDIS_QUARANTINE                "quarantine:"
#define REDIS_USER_BLOCK                "block:"
#define REDIS_BLACKLIST                 "blacklist:"
#define REDIS_FIELD_STATUS              "status"
#define REDIS_FIELD_CURRENT             "current"
#define REDIS_FIELD_TOTAL               "total"
#define REDIS_FIELD_RATE               "rate"

#if USE_REDIS_CLUSTER
    struct redisClusterContext;

#define REDIS_COMMAND    redisClusterCommand

#else
    struct redisContext;

#define REDIS_COMMAND    redisCommand

#endif

#ifndef CountArray
#define CountArray(a)   (sizeof(a)/sizeof(a[0]))
#endif//CountArray

namespace redis
{
	class RedisValItem
	{
	public:
		RedisValItem() {}
		virtual ~RedisValItem() {}

	public:
        int asInt()
        {
            if(val.empty())
                return 0;
            else
                return (stoi(val));
		}

        unsigned int asUInt()
        {
            if(val.empty())
                return 0;
            else
                return ((unsigned int)asInt());
		}

        long asLong()
        {
            if(val.empty())
                return 0;
            else
                return (stol(val));
		}

        int64_t asInt64()
        {
            if(val.empty())
                return 0;
            else
                return (stoll(val));
        }

        double asDouble()
        {
            if(val.empty())
                return 0.0;
            else
                return (stod(val));
		}

        // get the string value.
        string asString()
        {
            return val;
		}

	public:
		RedisValItem& operator=(string value)
		{
			val = value;
			return *this;
        }

		RedisValItem& operator=(long double value)
		{
			val = to_string(value);
			return *this;
		}

	protected:
		string val;
	};

	class RedisValue
	{
	public:
        RedisValue() {}
		virtual ~RedisValue() {}
		RedisValItem& operator[](string key)
		{
			map<string,RedisValItem>::iterator iter = listval.find(key);
            if (iter == listval.end())
            {
				listval[key]="";
			}

			RedisValItem& val = listval[key];
			return val;
		}

        void reset()
        {
            listval.clear();
        }

        bool empty()
        {
			return (listval.size()<=0);
		}

        // try to get the special map content now.
        map<string,redis::RedisValItem>& get()
        {
            return listval;
        }

	protected:
        map<string, RedisValItem> listval;
	};
}

// redis client.
class RedisClient
{

public:
    RedisClient();

    virtual ~RedisClient();

public:
    bool initRedisCluster(string ip, string password = "");
    bool initRedisCluster(string ip, map<string, string> &addrMap, string password = "");
    bool ReConnect();
    int  getMasterAddr(const vector<string> &addVec, struct timeval timeOut,string& masterIp, int& masterPort);

    bool get(string key, string &value);
    bool set(string key, string value, int timeout = 0);
    bool del(string key);
    int TTL(string key);
    bool exists(string key);
    bool persist(string key);

    bool hget(string key, string field, string &value);
    bool hset(string key, string field, string value, int timeout=0);

    bool hmget(string key, string* fields, int count, redis::RedisValue& redisValue);
    bool hmset(string key, redis::RedisValue& redisValue, int timeout=0);

    bool hmget(string key, vector<string>fields, vector<string> &values);
    bool hmset(string key, vector<string>fields, vector<string>values, int timeout=0);
    bool hmset(string key, map<string,string> fields,int timeout=0);
    bool hdel(string key, string field);
    bool exists(string key, string field);

    bool hincrby(string key, string field, int64_t inc, int64_t* result);
    bool hincrby_float(string key, string field, double inc, double* result);

    bool resetExpired(string key, int timeout = 60 * 3/*=MAX_USER_ONLINE_INFO_IDLE_TIME*/);
    bool resetExpiredEx(string key, int timeout=1000);
    
    // add by caiqing
    //推送公共消息
    void pushPublishMsg(int msgId,string msg);
    //订阅公共消息
    void subscribePublishMsg(int msgId,function<void(string)> func);
    // List 操作
//     bool lremCmd(eRedisKey keyId, int count, string value);
//     bool rpopCmd(eRedisKey keyId,string &lastElement);
//     bool lpushCmd(eRedisKey keyId,string value,long long &len);
//     bool lrangeCmd(eRedisKey keyId,vector<string> &list,int end,int start = 0); 
    // 集合操作
//     bool sremCmd(eRedisKey keyId,string value);  
//     bool saddCmd(eRedisKey keyId,string value); 
    //返回集合中的所有的成员（性能损耗大，适用于数量少数据）
//     bool smembersCmd(eRedisKey keyId,vector<string> &list); 
//     bool delnxCmd(eRedisKey keyId,string & lockValue);
//     int setnxCmd(eRedisKey keyId, string & value,int timeout);
private:
    bool lrem(string key, int count, string value);
    bool rpop(string key, string &values);
    bool sadd(string key, string value);        //向key的添加value值
    bool sismember(string key, string value);   //判断 member 元素是否是集合 key 的成员
    bool srem(string key, string value);        //删除key值中的value值
    bool smembers(string key, vector<string> &list);    //返回集合中的所有的成员
    //add end
private:
    bool blpop(string key, string &value, int timeOut);
    bool rpush(string key, string value);
    bool lpush(string key, string value, long long int &len);
    bool lrange(string key, int startIdx, int endIdx,vector<string> &values);
    bool ltrim(string key, int startIdx, int endIdx);
    bool llen(string key,int32_t &value);


public:
    bool SetUserOnlineInfo(int64_t userId, uint32_t nGameId, uint32_t nRoomId);
    bool GetUserOnlineInfo(int64_t userId, uint32_t &nGameId, uint32_t &nRoomId);
    bool SetUserOnlineInfoIP(int64_t userId, string ip);
    bool GetUserOnlineInfoIP(int64_t userId, string &ip);
    bool ResetExpiredUserOnlineInfo(int64_t userId,int timeout = 60 * 3/*=MAX_USER_ONLINE_INFO_IDLE_TIME*/);
    bool ExistsUserOnlineInfo(int64_t userId);
    bool DelUserOnlineInfo(int64_t userId);
    int TTLUserOnlineInfo(int64_t userId);
    bool GetGameServerplayerNum(vector<string> &serverValues,uint64_t &nTotalCount);
    bool GetGameRoomplayerNum(vector<string> &serverValues,map<string,uint64_t>& mapPlayerNum);
    bool GetGameAgentPlayerNum(vector<string> &keys,vector<string> &values);
public:
//    bool setUserLoginInfo(int64_t userId, Global_UserBaseInfo& userinfo);
//    bool getUserLoginInfo(int64_t userId, Global_UserBaseInfo& userinfo);

    bool GetUserLoginInfo(int64_t userId, string field0, string &value0);
    bool SetUserLoginInfo(int64_t userId, string field, const string &value);

    bool ResetExpiredUserLoginInfo(int64_t userId);
    bool ExistsUserLoginInfo(int64_t userId);
    bool DeleteUserLoginInfo(int64_t userId);
//    int TTLUserLoginInfo(int64_t userId);
    bool AddToMatchedUser(int64_t userId, int64_t blockUser);
    bool GetMatchBlockList(int64_t userId,vector<string> &list);
    bool RemoveQuarantine(int64_t userId);
    bool AddQuarantine(int64_t userId);

public:

    //================Message Publish Subscribe============
    void publishRechargeScoreMessage(string msg);
    void subscribeRechargeScoreMessage(function<void(string)> func);

    void publishRechargeScoreToProxyMessage(string msg);
    void subscribeRechargeScoreToProxyMessage(function<void(string)> func);

    void publishRechargeScoreToGameServerMessage(string msg);
    void subscribeRechargeScoreToGameServerMessage(function<void(string)> func);

    void publishExchangeScoreMessage(string msg);
    void subscribeExchangeScoreMessage(function<void(string)> func);

    void publishExchangeScoreToProxyMessage(string msg);
    void subscribeExchangeScoreToProxyMessage(function<void(string)> func);

    void publishExchangeScoreToGameServerMessage(string msg);
    void subscribeExchangeScoreToGameServerMessage(function<void(string)> func);

    void publishUserLoginMessage(string msg);
    void subscribeUserLoginMessage(function<void(string)> func);

    void publishUserKillBossMessage(string msg);
    void subscribeUserKillBossMessage(function<void(string)> func);

    void publishNewChatMessage(string msg);
    void subscribeNewChatMessage(function<void(string)> func);

    void publishNewMailMessage(string msg);
    void subscribeNewMailMessage(function<void(string)> func);

    void publishNoticeMessage(string msg);
    void subscribeNoticeMessage(function<void(string)> func);

    void publishStopGameServerMessage(string msg);
    void subscribeStopGameServerMessage(function<void(string)> func);

    void publishRefreashConfigMessage(string msg);
    void subsreibeRefreashConfigMessage(function<void(string)> func);

    void publishOrderScoreMessage(string msg);
    void subsreibeOrderScoreMessage(function<void(string)> func);

    void unsubscribe();
    void getSubMessage();
    void startSubThread();

private:
    bool auth(string pass);

    void publish(string channel, string msg);
    void subscribe(string channel);

public:
    bool PushSQL(string sql);
    bool POPSQL(string &sql, int timeOut);
    bool BlackListHget(string key, string keyson,redis::RedisValue& values,map<string,int16_t> &usermap);
private:
    shared_ptr<thread> m_redis_pub_sub_thread;
    map<string, function<void(string)> > m_sub_func_map;

private:
#if USE_REDIS_CLUSTER
    redisClusterContext *m_redisClientContext;
#else
    redisContext* m_redisClientContext;
#endif
    string        m_ip;
};




//    int setVerifyCode(string phoneNum, int type);  //0 getVerifycode ok   1 already set code 2 error
//    int getVerifyCode(string phoneNum, int type, string &verifyCode);  //0 getVerifycode ok   1 noet exists 2 error;
//    void setVerifyCode(string phoneNum, int type, string &verifyCode);
//    bool existsVerifyCode(string phoneNum, int type);

//    bool setUserLoginInfo(int64_t userId, string &account, string &password, string &dynamicPassword, int temp,
//                                     string &machineSerial, string &machineType, int nPlatformId, int nChannelId);

//    bool setUserIdGameServerInfo(int64_t userId, string ip);
//    bool getUserIdGameServerInfo(int64_t userId, string &ip);
//    bool resetExpiredUserIdGameServerInfo(int64_t userId);
//    bool existsUserIdGameServerInfo(int64_t userId);
//    bool delUserIdGameServerInfo(int64_t userId);
//    bool persistUserIdGameServerInfo(int64_t userId);
//    int TTLUserIdGameServerInfo(int64_t userId);




#endif // REDISCLIENT_H
