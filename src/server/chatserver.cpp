#include "chatserver.hpp"
#include "json.hpp"
#include "chatservice.hpp"

#include <iostream>
#include <functional>
#include <string>

using namespace std;
using namespace placeholders;

using json = nlohmann::json;

// 构造函数
ChatServer::ChatServer(EventLoop* loop,
            const InetAddress& listenAddr,
            const string& nameArg): _server(loop,listenAddr,nameArg), _loop(loop)
{
    // 注册连接回调
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection,this,_1));
    // 注册消息回调
    _server.setMessageCallback(std::bind(&ChatServer::onMessage,this,_1,_2,_3));
    // 设置线程数量   
    _server.setThreadNum(6);
}

// 启动服务
void ChatServer::start()
{
    _server.start();
}

// 上报链接相关信息的回调函数
void ChatServer::onConnection(const TcpConnectionPtr & conn)
{
    // 客户端断开连接
    if(!conn->connected()){
        
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}

// 上报读写事件相关信息的回调函数 —— 在多线程环境中运行的，要注意线程安全
void ChatServer::onMessage(const TcpConnectionPtr & conn,
                            Buffer * buffer,
                            Timestamp time)
{
    // 把onMessage函数上报的buffer内容转为string
    string buf  = buffer->retrieveAllAsString();

    cout<<buf<<endl;

    json js = json::parse(buf);
    // 通过单例获得回调函数句柄
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    // 处理对应的业务
    msgHandler(conn,js,time);
    
}



