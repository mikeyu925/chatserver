#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <mutex>
#include "usermodel.hpp"
#include "json.hpp"
#include "offlinemsgmodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"

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

    // 数据操作类
    UserModel _userModel; // 专门操作User对象的类
    FriendModel _friendModel;
    GroupModel _groupModel;
    // 离线消息操作类
    OfflineMsgModel _offlineMsgModel;

    // 存储在线用户的通信连接
    unordered_map<int,TcpConnectionPtr> _userConnMap;

    // 互斥锁 保证userConnMap线程安全
    mutex _connMutex;
    
public:
    // 暴露一个接口获得实例，定义为static,可以通过类直接获得
    static ChatService * instance();
    // 处理登陆业务
    void login(const TcpConnectionPtr & conn,json & js, Timestamp time);
    // 处理注册业务
    void reg(const TcpConnectionPtr & conn,json & js, Timestamp time);
    // 一对一聊天
    void oneToOneChat(const TcpConnectionPtr & conn,json & js, Timestamp time);
    // 添加好友
    void addFriend(const TcpConnectionPtr & conn,json & js, Timestamp time);
    // 处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);
    // 处理服务器异常退出
    void reset();
    // 创建群组业务
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 加入群组业务
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 群组聊天业务
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 处理注销业务
    void loginout(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 获取消息对应的处理器
    MsgHandler getHandler(int msgid);

    // 默认析构函数
    ~ChatService();
};





#endif