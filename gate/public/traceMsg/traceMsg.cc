#include <sstream>
#include "traceMsg.h"

#include "proto/Game.Common.pb.h"
#include "proto/ProxyServer.Message.pb.h"
#include "proto/HallServer.Message.pb.h"
//#include "proto/GameServer.Message.pb.h"
//#include "proto/MatchServer.Message.pb.h"

//mainid
#define MY_MAINID_MAP(XX) \
	XX(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_ID_BEGIN, "") \
	XX(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_PROXY, "网关服") \
	XX(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL, "大厅服") \
	XX(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER, "游戏服") \
	XX(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_LOGIC, "逻辑服") \
	XX(::Game::Common::MAINID::MAIN_MESSAGE_PROXY_TO_HALL, "大厅服") \
	XX(::Game::Common::MAINID::MAIN_MESSAGE_HALL_TO_PROXY, "网关服") \
	XX(::Game::Common::MAINID::MAIN_MESSAGE_PROXY_TO_GAME_SERVER, "游戏服") \
	XX(::Game::Common::MAINID::MAIN_MESSAGE_GAME_SERVER_TO_PROXY, "网关服") \
	XX(::Game::Common::MAINID::MAIN_MESSAGE_HTTP_TO_SERVER, "服务器") \
	XX(::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_MATCH_SERVER, "比赛服")
MY_TAB_MAP(mainid_, MY_MAINID_MAP);

//subid CLIENT_TO_SERVER Client <-> GameServer/HallServer
#define MY_SUBID_CLIENT_TO_SERVER_MAP(XX) \
	XX(::Game::Common::MESSAGE_CLIENT_TO_SERVER_SUBID::MESSAGE_CLIENT_TO_SERVER_SUBID_BEGIN, "") \
	XX(::Game::Common::MESSAGE_CLIENT_TO_SERVER_SUBID::KEEP_ALIVE_REQ, "心跳 SYN") \
	XX(::Game::Common::MESSAGE_CLIENT_TO_SERVER_SUBID::KEEP_ALIVE_RES, "心跳 ACK")
MY_TAB_MAP(subid_client_to_server_, MY_SUBID_CLIENT_TO_SERVER_MAP);

//subid CLIENT_TO_PROXY Client <-> ProxyServer
#define MY_SUBID_CLIENT_TO_PROXY_MAP(XX) \
	XX(::Game::Common::MESSAGE_CLIENT_TO_PROXY_SUBID::MESSAGE_CLIENT_TO_PROXY_SUBID_BEGIN, "") \
	XX(::Game::Common::MESSAGE_CLIENT_TO_PROXY_SUBID::CLIENT_TO_PROXY_GET_AES_KEY_MESSAGE_REQ, "查询AESKey REQ") \
	XX(::Game::Common::MESSAGE_CLIENT_TO_PROXY_SUBID::CLIENT_TO_PROXY_GET_AES_KEY_MESSAGE_RES, "查询AESKey RSP") \
	XX(::Game::Common::MESSAGE_CLIENT_TO_PROXY_SUBID::CLIENT_TO_PROXY_BIND_USER_CONNECTION_MESSAGE_REQ, "绑定用户连接REQ")	\
	XX(::Game::Common::MESSAGE_CLIENT_TO_PROXY_SUBID::CLIENT_TO_PROXY_BIND_USER_CONNECTION_MESSAGE_RES, "绑定用户连接RSP")	\
	XX(::Game::Common::MESSAGE_CLIENT_TO_PROXY_SUBID::CLIENT_TO_PROXY_BIND_GAME_SERVER_MESSAGE_REQ, "绑定游戏服REQ")	\
	XX(::Game::Common::MESSAGE_CLIENT_TO_PROXY_SUBID::CLIENT_TO_PROXY_BIND_GAME_SERVER_MESSAGE_RES, "绑定游戏服RSP")	\
	XX(::Game::Common::MESSAGE_CLIENT_TO_PROXY_SUBID::PROXY_NOTIFY_SHUTDOWN_USER_CLIENT_MESSAGE_NOTIFY, "关闭客户端通知")	\
	XX(::Game::Common::MESSAGE_CLIENT_TO_PROXY_SUBID::PROXY_NOTIFY_PUBLIC_NOTICE_MESSAGE_NOTIFY, "订阅通知") \
	XX(::Game::Common::MESSAGE_CLIENT_TO_PROXY_SUBID::PROXY_NOTIFY_USER_ORDER_SCORE_MESSAGE, "用户上下分通知!!!")
MY_TAB_MAP(subid_client_to_proxy_, MY_SUBID_CLIENT_TO_PROXY_MAP);

//subid CLIENT_TO_HALL Client <-> HallServer
#define MY_SUBID_CLIENT_TO_HALL_MAP(XX) \
	XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::MESSAGE_CLIENT_TO_HALL_SUBID_BEGIN, "") \
	XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_LOGIN_MESSAGE_REQ, "登陆大厅REQ") \
	XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_LOGIN_MESSAGE_RES, "登陆大厅RSP") \
	XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_GAME_ROOM_INFO_REQ, "查询游戏房间信息REQ")	\
	XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_GAME_ROOM_INFO_RES, "查询游戏房间信息RSP")	\
	XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_GAME_SERVER_MESSAGE_REQ, "查询游戏服务器信息REQ") \
	XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_GAME_SERVER_MESSAGE_RES, "查询游戏服务器信息RSP") \
	XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_PLAYING_GAME_INFO_MESSAGE_REQ, "查询游戏信息REQ") \
	XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_PLAYING_GAME_INFO_MESSAGE_RES, "查询游戏信息RSP") \
	XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_SET_HEAD_MESSAGE_REQ, "修改头像REQ") \
	XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_SET_HEAD_MESSAGE_RES, "修改头像RSP") \
	XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_SET_NICKNAME_MESSAGE_REQ, "修改昵称REQ") \
	XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_SET_NICKNAME_MESSAGE_RES, "修改头像RSP") \
	XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_USER_SCORE_MESSAGE_REQ, "查询积分REQ") \
	XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_USER_SCORE_MESSAGE_RES, "查询积分RSP") \
	XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_PLAY_RECORD_REQ, "查询游戏记录REQ") \
	XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_PLAY_RECORD_RES, "查询游戏记录RSP") \
	XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_ROOM_PLAYER_NUM_REQ, "查询房间玩家数REQ") \
	XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_ROOM_PLAYER_NUM_RES, "查询房间玩家数RSP") \
	XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_MATCH_ROOM_INFO_REQ, "查询比赛房间信息REQ") \
	XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_MATCH_ROOM_INFO_RES, "查询比赛房间信息RSP") \
	XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_MATCH_RECORD_REQ, "查询比赛记录REQ") \
	XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_MATCH_RECORD_RES, "查询比赛记录RSP") \
	XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_MATCH_BEST_RECORD_REQ, "查询比赛最佳记录REQ") \
	XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_MATCH_BEST_RECORD_RES, "查询比赛最佳记录RSP") \
	XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_LUCKY_GAME_REQ, "查询转盘积分REQ") \
	XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_GET_LUCKY_GAME_RES, "查询转盘积分RSP") \
	XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_SWICTH_LUCKY_GAME_REQ, "") \
	XX(::Game::Common::MESSAGE_CLIENT_TO_HALL_SUBID::CLIENT_TO_HALL_SWICTH_LUCKY_GAME_RES, "")
MY_TAB_MAP(subid_client_to_hall_, MY_SUBID_CLIENT_TO_HALL_MAP);

#if 0

//subid CLIENT_TO_GAME_SERVER Client <-> GameServer
#define MY_SUBID_CLIENT_TO_GAME_SERVER_MAP(XX) \
	XX(::GameServer::SUBID::SUBID_BEGIN, "") \
	XX(::GameServer::SUBID::SUB_C2S_ENTER_ROOM_REQ, "进入房间REQ") \
	XX(::GameServer::SUBID::SUB_S2C_ENTER_ROOM_RES, "进入房间RSP") \
	XX(::GameServer::SUBID::SUB_S2C_USER_ENTER_NOTIFY, "进入房间通知") \
	XX(::GameServer::SUBID::SUB_S2C_USER_SCORE_NOTIFY, "用户积分通知") \
	XX(::GameServer::SUBID::SUB_S2C_USER_STATUS_NOTIFY, "用户状态通知") \
	XX(::GameServer::SUBID::SUB_C2S_USER_READY_REQ, "用户准备REQ") \
	XX(::GameServer::SUBID::SUB_S2C_USER_READY_RES, "用户准备RSP") \
	XX(::GameServer::SUBID::SUB_C2S_USER_LEFT_REQ, "用户离开REQ") \
	XX(::GameServer::SUBID::SUB_S2C_USER_LEFT_RES, "用户离开RSP") \
	XX(::GameServer::SUBID::SUB_C2S_CHANGE_TABLE_REQ, "用户换桌REQ") \
	XX(::GameServer::SUBID::SUB_S2C_CHANGE_TABLE_RES, "用户换桌RSP") \
	XX(::GameServer::SUBID::SUB_C2S_USER_RECHARGE_OK_REQ, "用户充值成功 REQ") \
	XX(::GameServer::SUBID::SUB_S2C_USER_RECHARGE_OK_RES, "用户充值成功 RSP") \
	XX(::GameServer::SUBID::SUB_C2S_USER_EXCHANGE_FAIL_REQ, "用户充值失败 REQ") \
	XX(::GameServer::SUBID::SUB_S2C_USER_EXCHANGE_FAIL_RES, "用户充值失败 RSP") \
	XX(::GameServer::SUBID::SUB_C2S_CHANGE_TABLE_SIT_CHAIR_REQ, "用户入座REQ") \
	XX(::GameServer::SUBID::SUB_S2C_CHANGE_TABLE_SIT_CHAIR_RES, "用户入座RSP") \
	XX(::GameServer::SUBID::SUB_S2C_PLAY_IN_OTHERROOM, "用户游戏中") \
	XX(::GameServer::SUBID::SUB_GF_SYSTEM_MESSAGE, "系统消息")
MY_TAB_MAP(subid_client_to_game_server_, MY_SUBID_CLIENT_TO_GAME_SERVER_MAP);

//subid CLIENT_TO_GAME_LOGIC Client <-> GameServer libGame_xxx.so
#endif

//subid PROXY_TO_HALL ProxyServer -> HallServer
#define MY_SUBID_PROXY_TO_HALL_MAP(XX) \
	XX(::Game::Common::MESSAGE_PROXY_TO_HALL_SUBID::HALL_ON_USER_OFFLINE, "用户离线")
MY_TAB_MAP(subid_proxy_to_hall_, MY_SUBID_PROXY_TO_HALL_MAP);

//subid HALL_TO_PROXY HallServer -> ProxyServer

//subid PROXY_TO_GAME_SERVER ProxyServer -> GameServer
#define MY_SUBID_PROXY_TO_GAME_SERVER_MAP(XX) \
	XX(::Game::Common::MESSAGE_PROXY_TO_GAME_SERVER_SUBID::GAME_SERVER_ON_USER_OFFLINE, "用户离线")
MY_TAB_MAP(subid_proxy_to_game_server_, MY_SUBID_PROXY_TO_GAME_SERVER_MAP);

//subid GAME_SERVER_TO_PROXY GameServer -> ProxyServer
#define MY_SUBID_GAME_SERVER_TO_PROXY_MAP(XX) \
	XX(::Game::Common::MESSAGE_GAME_SERVER_TO_PROXY_SUBID::MESSAGE_GAME_SERVER_TO_PROXY_SUBID_BEGIN, "") \
	XX(::Game::Common::MESSAGE_GAME_SERVER_TO_PROXY_SUBID::PROXY_NOTIFY_KILL_BOSS_MESSAGE_REQ, "踢出玩家")
MY_TAB_MAP(subid_game_server_to_proxy_, MY_SUBID_GAME_SERVER_TO_PROXY_MAP);

//subid HTTP_TO_SERVER
#define MY_SUBID_HTTP_TO_SERVER_MAP(XX) \
	XX(::Game::Common::MESSAGE_HTTP_TO_SERVER_SUBID::MESSAGE_NOTIFY_REPAIR_SERVER, "Http修复")
MY_TAB_MAP(subid_http_to_server_, MY_SUBID_HTTP_TO_SERVER_MAP);

#if 0

//subid CLIENT_TO_MATCH_SERVER Client <-> MatchServer
#define MY_SUBID_CLIENT_TO_MATCH_SERVER_MAP(XX) \
	XX(::MatchServer::SUBID::SUBID_BEGIN, "") \
	XX(::MatchServer::SUBID::SUB_C2S_ENTER_MATCH_REQ, "报名 REQ") \
	XX(::MatchServer::SUBID::SUB_C2S_LEFT_MATCH_REQ, "退赛 REQ") \
	XX(::MatchServer::SUBID::SUB_C2S_USER_RANK_REQ, "查询比赛排行REQ")	\
	XX(::MatchServer::SUBID::SUB_S2C_ENTER_MATCH_RESP, "报名 RSP") \
	XX(::MatchServer::SUBID::SUB_S2C_LEFT_MATCH_RESP, "退赛 RSP") \
	XX(::MatchServer::SUBID::SUB_S2C_USER_ENTER_NOTIFY, "报名 notify")	\
	XX(::MatchServer::SUBID::SUB_S2C_MATCH_SCENE, "比赛场景") \
	XX(::MatchServer::SUBID::SUB_S2C_MATCH_PLAYER_UPDATE, "更新比赛人数") \
	XX(::MatchServer::SUBID::SUB_S2C_MATCH_RANK_UPDATE, "更新比赛排行") \
	XX(::MatchServer::SUBID::SUB_S2C_MATCH_FINISH, "比赛结束") \
	XX(::MatchServer::SUBID::SUB_S2C_USER_RANK_RESP, "查询比赛排行RSP") \
	XX(::MatchServer::SUBID::SUB_S2C_UPGRADE, "比赛升级")
MY_TAB_MAP(subid_client_to_match_server_, MY_SUBID_CLIENT_TO_MATCH_SERVER_MAP);

#endif

//strMessageID 格式化输入mainID，subID
std::string const strMessageID(
	uint8_t mainID, uint8_t subID,
	bool trace_hall_heartbeat,
	bool trace_game_heartbeat) {
	std::string strMainID, strMainDesc, strSubID, strSubDesc;
	//格式化mainID
	MY_CMD_DESC(mainID, mainid_, strMainID, strMainDesc);
	//格式化subID
	switch (mainID)
	{
	case ::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_PROXY: {
		//CLIENT_TO_SERVER
		//MY_CMD_DESC(subID, subid_client_to_server_, strSubID, strSubDesc);
		//CLIENT_TO_PROXY
		MY_CMD_DESC(subID, subid_client_to_proxy_, strSubID, strSubDesc);
		break;
	}
	case ::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL: {
		//CLIENT_TO_SERVER 大厅服心跳包
		if (trace_hall_heartbeat) {
			MY_CMD_DESC(subID, subid_client_to_server_, strSubID, strSubDesc);
		}
		//CLIENT_TO_HALL
		if (strSubID.empty()) {
			MY_CMD_DESC(subID, subid_client_to_hall_, strSubID, strSubDesc);
		}
		break;
	}
// 	case ::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER: {
// 		//CLIENT_TO_SERVER 游戏服心跳包
// 		if (trace_game_heartbeat) {
// 			MY_CMD_DESC(subID, subid_client_to_server_, strSubID, strSubDesc);
// 		}
// 		//CLIENT_TO_GAME_SERVER
// 		if (strSubID.empty()) {
// 			MY_CMD_DESC(subID, subid_client_to_game_server_, strSubID, strSubDesc);
// 		}
// 		break;
// 	}
// 	case ::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_LOGIC: {
// 		//CLIENT_TO_GAME_LOGIC 针对每个字游戏
// 
// 		break;
// 	}
	case ::Game::Common::MAINID::MAIN_MESSAGE_PROXY_TO_HALL: {
		//PROXY_TO_HALL
		MY_CMD_DESC(subID, subid_proxy_to_hall_, strSubID, strSubDesc);
		break;
	}
	case ::Game::Common::MAINID::MAIN_MESSAGE_HALL_TO_PROXY: {
		//HALL_TO_PROXY
		break;
	}
	case ::Game::Common::MAINID::MAIN_MESSAGE_PROXY_TO_GAME_SERVER: {
		//PROXY_TO_GAME_SERVER
		MY_CMD_DESC(subID, subid_proxy_to_game_server_, strSubID, strSubDesc);
		break;
	}
// 	case ::Game::Common::MAINID::MAIN_MESSAGE_GAME_SERVER_TO_PROXY: {
// 		//GAME_SERVER_TO_PROXY
// 		MY_CMD_DESC(subID, subid_game_server_to_proxy_, strSubID, strSubDesc);
// 		break;
// 	}
	case ::Game::Common::MAINID::MAIN_MESSAGE_HTTP_TO_SERVER: {
		//HTTP_TO_SERVER
		MY_CMD_DESC(subID, subid_http_to_server_, strSubID, strSubDesc);
		break;
	}
// 	case ::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_MATCH_SERVER: {
// 		//CLIENT_TO_MATCH_SERVER
// 		MY_CMD_DESC(subID, subid_client_to_match_server_, strSubID, strSubDesc);
// 		break;
// 	}
	default:
		break;
	}
	//mainid CLIENT_TO_HALL/CLIENT_TO_GAME_SERVER
	if ((mainID == ::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_HALL ||
		mainID == ::Game::Common::MAINID::MAIN_MESSAGE_CLIENT_TO_GAME_SERVER) &&
		strSubID.empty()) {
		//屏蔽跟踪心跳包 ///
		return "";
	}
	//字符串格式：
	//strMainID:mainID[strMainDesc]
	//strSubID:subID[strSubDesc]
	std::stringstream ss1, ss2;
	ss1 << (int)mainID;
	strMainID += ":" + ss1.str();
	ss2 << (int)subID;
	strSubID += ":" + ss2.str();
	return
		"\n" +
		(strMainDesc.empty() ? strMainID : (strMainID + "[" + strMainDesc + "]")) +
		"\n" +
		(strSubDesc.empty() ? strSubID : (strSubID + "[" + strSubDesc + "]"));
}