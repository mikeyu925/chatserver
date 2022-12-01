#include "chatservice.hpp"
#include "public.hpp"
#include <vector>
#include <muduo/base/Logging.h>
using namespace std;
using namespace muduo;

// 单例模式获取实例对象
ChatService * ChatService::instance(){
    static ChatService service;
    return &service;
}

ChatService::ChatService()
{
    // 用户基本业务管理相关事件处理回调函数注册
    _msgHandlerMap.insert({LOGIN_MSG,std::bind(&ChatService::login,this,_1,_2,_3)});    // 登录
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg,this, _1, _2, _3)});   // 注册
    
}

ChatService::~ChatService()
{

}

// 获取消息对应的处理器
MsgHandler ChatService::getHandler(int msgid){
    // 记录报错日志，msgid没有对应的事件处理回调函数
    auto it = _msgHandlerMap.find(msgid);
    if(it == _msgHandlerMap.end()){
        // 返回一个默认处理器，空操作
        return [=](const TcpConnectionPtr & conn,json & js, Timestamp time){
            LOG_ERROR << "msgid:" <<msgid<<"can not find handler!"; // 不用加<<endl; 自带
        };
    }else{
        // 返回对应的信息处理句柄
        return _msgHandlerMap[msgid];
    }
}

// 处理登陆业务
void ChatService::login(const TcpConnectionPtr & conn,json & js, Timestamp time)
{
    LOG_INFO << "login !";
}
// 处理注册业务
void ChatService::reg(const TcpConnectionPtr & conn,json & js, Timestamp time)
{
    LOG_INFO << "regist !";
}


