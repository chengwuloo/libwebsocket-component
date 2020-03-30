#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <zookeeper/zookeeper.h>


using namespace std;


//摘自zookeeper源文件 zk_adptor.h
/* zookeeper event type constants */
#define CREATED_EVENT_DEF 1
#define DELETED_EVENT_DEF 2
#define CHANGED_EVENT_DEF 3
#define CHILD_EVENT_DEF 4
#define SESSION_EVENT_DEF -1
#define NOTWATCHING_EVENT_DEF -2

//摘自zookeeper源文件 zk_adptor.h
/* zookeeper state constants */
#define EXPIRED_SESSION_STATE_DEF -112
#define AUTH_FAILED_STATE_DEF -113
#define CONNECTING_STATE_DEF 1
#define ASSOCIATING_STATE_DEF 2
#define CONNECTED_STATE_DEF 3
#define NOTCONNECTED_STATE_DEF 999


#define destroyPointer(pointer)\
    delete pointer;\
    pointer = nullptr;

//static const int IGNORE_VERSION = -1;



class ZookeeperClientUtils
{
public:
    ZookeeperClientUtils();


    static void printString(const string &name);
    static void printStringPair(const pair<string, string> &stringPair);

    static void transStringVector2VectorString(const String_vector &children, vector<string> &vecString);
    static void printPathList(const vector<string> &nodeNameVector);
    static void printPathValueList(const map<string, string> &pathValueMap);

    static void transACLVector2VectorACL(const ACL_vector &childs, vector<ACL> &vectorACL);

    static const string watcherEventType2String(int type);
    static const string state2String(int state);


};




