#ifndef BACKCODE_H
#define BACKCODE_H

/************************************************
                   API请求的返回码
*************************************************/
enum eApiErrorCode
{
    // 执行成功
    NoError = 0,
    // 内部异常或未执行任务
    InsideErrorOrNonExcutive = -1,
    // 代理ID不存在或代理已停用
    GameHandleProxyIDError = -2,
    // 代理MD5校验码错误
    GameHandleProxyMD5CodeError = -3,
    // 参数转码或代理解密校验码错误
    GameHandleProxyDESCodeError = -4,
    // 传输参数错误
    GameHandleParamsError = -5,
    // 操作类型参数错误
    GameHandleOperationTypeError = -6,
    // 玩家账号不存在
    GameHandleUserNotExistsError = -7,
    // 站点编码为空
    GameHandleUserLineCodeNull = -8,
    // 站点编码不存在
    GameHandleUserLineCodeNotExists = -9,
    // 请求已失效
    GameHandleRequestInvalidation = -10,

    /*********************GameUserHandleErrorSet**********************/
    // 玩家注册或登录成功
    // GameUserHandleNoError = 0,
    // 创建平台玩家账号信息错误
    GameUserHandleCreateUserError = 11,
    // 更新平台玩家账号信息错误
    GameUserHandleUpdateUserError = 12,
    // 玩家账号在本平台封停
    GameUserHandleUserAccountStatusError = 13,
    // 玩家登录日志存储错误
    GameUserHandleUserLoginLogError = 14,
    // 玩家信息Redis写入错误
    GameUserHandleUserRedisWriteError = 15,
    // 玩家账号信息为空错误
    GameUserHandleUseAccountNullError = 16,
    // 玩家账号信息不存在
    GameUserHandleUseAccountNotExistsError = 17,
    // 玩家账号信息已存在
    GameUserHandleUseAccountIsExistsError = 18,
    // play_record 表的 CardValue 写入规则错误
    GameCardValueWriteError = 19,

    /*********************CanSubScoreCheckErrorSet**********************/
    // 玩家可下分余额查询成功
    CanSubScoreCheckNoError = 0,
    // 可下分余额查询_玩家账号不存在
    CanSubScoreUserAccountNotExistsError = 21,

    /*********************AddScoreErrorSet**********************/
    // 玩家上分成功
    // AddScoreHandleNoError = 0,
    // 玩家不存在
    AddScoreHandleUserNotExistsError = 31,
    // 玩家上分失败
    AddScoreHandleInsertDataError = 32,
    // 玩家上分订单已存在
    AddScoreHandleInsertDataOrderIDExists = 33,
    // 玩家游戏中
    AddScoreHandleInsertDataUserInGaming = 34,
    // 玩家上分超出代理现有总分值
    AddScoreHandleInsertDataOverScore = 35,
    // 玩家上分等待超时
    AddScoreHandleInsertDataOutTime = 36,

    /*********************SubScoreErrorSet**********************/
    // 玩家下分成功
    // SubScoreHandleNoError = 0,
    // 玩家不存在
    SubScoreHandleUserNotExistsError = 41,
    // 玩家下分失败
    SubScoreHandleInsertDataError = 42,
    // 玩家下分订单已存在
    SubScoreHandleInsertOrderIDExists = 43,
    // 玩家正在游戏中，不能下分
    SubScoreHandleInsertDataUserInGaming = 44,
    // 玩家下分超出玩家现有总分值
    SubScoreHandleInsertDataOverScore = 45,
    // 玩家下分等待超时
    SubScoreHandleInsertDataOutTime = 46,
    // 玩家正在进入游戏中
    SubScoreHandleInsertDataUserJoiningGame = 47,

    /*********************OrderScoreCheckErrorSet**********************/
    // 玩家上下分订单信息查询成功
    // OrderScoreCheckNoError = 0,
    // 玩家不存在
    OrderScoreCheckUserNotExistsError = 51,
    // 订单不存在
    OrderScoreCheckOrderNotExistsError = 52,
    // 订单查询错误
    OrderScoreCheckDataError = 53,

    /*********************UserStatusCheckErrorSet**********************/
    // 玩家状态信息查询成功
    // UserStatusCheckNoError = 0,
    // 玩家不存在
    UserStatusCheckUserNotExistsError = 61,
    // 玩家状态查询错误
    UserStatusCheckDataError = 62,

    /*********************UserInfoCheckErrorSet**********************/
    // 玩家完整信息查询成功
    // UserInfoCheckNoError = 0,
    // 玩家不存在
    UserInfoCheckUserNotExistsError = 71,
    // 玩家信息查询错误
    UserInfoCheckDataError = 72,

    /*********************ProxyScoreCheckErrorSet**********************/
    // 踢玩家下线成功
    // ProxyScoreCheckNoError = 0,

    /*********************KickOutGameUserOffLineErrorSet**********************/
    // 踢玩家下线成功
    // KickOutGameUserOffLineNoError = 0,
    // 玩家不存在
    KickOutGameUserOffLineUserNotExistsError = 91,
    // 踢玩家下线失败
    KickOutGameUserOffLineError = 92,
    // 玩家账号已被封停
    KickOutGameUserOffLineUserAccountError = 93,

    /*********************GameRecordGetErrorSet**********************/
    // 注单拉取成功
    // GameRecordGetNoError = 0,
    // 无注单数据
    GameRecordGetNoDataError = 101,
};

/************************************************
                API请求的相关枚举
*************************************************/
enum eCooType
{
    //买分
    buyScore     = 1,         
    //信用  
    credit       = 2,          
};
enum eApiType
{
    OpUnknown = -1,
	OpAddScore = 2,
	OpSubScore = 3,
};

//IP访问白名单信息 ///
enum eApiVisit {
    //IP允许访问
    Enable = 0,
    //IP禁止访问
    Disable = 1,
};

//节点服务状态 ///
enum {
	//维护中
	ServiceRepairing = 0,
	//服务中
	ServiceRunning = 1,
};

#endif // BACKCODE_H
