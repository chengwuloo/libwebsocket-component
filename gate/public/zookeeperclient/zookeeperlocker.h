#pragma once

#include <zookeeper/zookeeper.h>

#include <memory>
#include <functional>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif

#include "zoo_lock.h"

#ifdef __cplusplus
}
#endif

using namespace std;



class ZookeeperLocker;

typedef function<void (int rc, const string &path, void *context,
                       const shared_ptr<ZookeeperLocker> &zkLockerPtr)>  LockerWatcherHandler;


/**
 * 监视器
 */
class ZkLockerWatcherOperateContext
{
public:
    ZkLockerWatcherOperateContext(zhandle_t *zh, const string &path,
                                  struct ACL_vector *acl, void *cbdata,
                                  const shared_ptr<ZookeeperLocker> &zkLockerPtr)
    {
        m_zkHandle = zh;
        m_path = path;
        m_acl = acl;
        m_context = cbdata;
        m_zkLockerPtr = zkLockerPtr;
    }

    zhandle_t *m_zkHandle;
    string m_path;
    ACL_vector *m_acl;
    void *m_context;
    shared_ptr<ZookeeperLocker> m_zkLockerPtr;

    LockerWatcherHandler m_lockerWatcherHandler;
};



class ZookeeperLocker : public enable_shared_from_this<ZookeeperLocker>
{
public:
    ZookeeperLocker(const ZookeeperLocker &) = delete;
    ZookeeperLocker & operator=(const ZookeeperLocker &) = delete;

    /**
     * ZookeeperLocker constructor
     * @param zh  session handler defined by yourself
     * @param path locker path
     * @param acl
     */
    ZookeeperLocker();

    /**
     * 释放资源
     */
    ~ZookeeperLocker();

    void initLocker(zhandle_t* zh, char* path, struct ACL_vector *acl,
                    LockerWatcherHandler handler, void* context);
    void destroyLocker();
    int lock();
    int unlock();

    char* getLockPath();
    int isOwnerLock();
    char* getLockId();

private:
    //watch handler
    static void zookeeperLockerWatcher(int rc, void* cbdata);

private:
//    zhandle_t *m_zkHandle = nullptr;
//    string m_path = nullptr;
//    ACL_vector *m_acl = nullptr;

    zkr_lock_mutex_t m_zkr_lock_mutex;

};

