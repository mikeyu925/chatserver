#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <mutex>

#include "json.hpp"


using namespace std;
using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;


// 定义 处理消息的事件回调方法类型
using MsgHandler = std::function<void (const TcpConnectionPtr & conn,json & js, Timestamp time)>;


// 单例聊天服务器业务类
class ChatService
{
private:
    // 设计成单例模式_  构造函数私有化
    ChatService();
    // 存储消息id和其对应的业务处理方法
    unordered_map<int,MsgHandler> _msgHandlerMap;
    
public:
    // 暴露一个接口获得实例，定义为static,可以通过类直接获得
    static ChatService * instance();
    // 处理登陆业务
    void login(const TcpConnectionPtr & conn,json & js, Timestamp time);
    // 处理注册业务
    void reg(const TcpConnectionPtr & conn,json & js, Timestamp time);


    // 获取消息对应的处理器
    MsgHandler getHandler(int msgid);

    // 默认析构函数
    ~ChatService();
};





#endif