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
    _msgHandlerMap.insert({LOGINOUT_MSG, std::bind(&ChatService::loginout, this, _1, _2, _3)}); // 登出
    _msgHandlerMap.insert({REG_MSG, std::bind(&ChatService::reg,this, _1, _2, _3)});   // 注册
    _msgHandlerMap.insert({ONE_CHAT_MSG,std::bind(&ChatService::oneToOneChat,this,_1,_2,_3)}); // 聊天
    _msgHandlerMap.insert({ADD_FRIEND_MSG,std::bind(&ChatService::addFriend,this,_1,_2,_3)}); // 添加好友

    // 群组业务管理相关事件处理回调注册
    _msgHandlerMap.insert({CREATE_GROUP_MSG, std::bind(&ChatService::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG, std::bind(&ChatService::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&ChatService::groupChat, this, _1, _2, _3)});

    // 连接redis服务器
    // if (_redis.connect())
    // {
    //     // 设置上报消息的回调
    //     _redis.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage, this, _1, _2));
    // }

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
    int id = js["id"].get<int>();
    string pwd = js["password"];
    // 检测输入用户名及密码是否正确
    User user = _userModel.query(id);
    if(user.getId() == id && user.getPwd() == pwd){
        // 用户已经登陆，不能重复登录
        if(user.getState() == "online"){
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "this account is online, can not login !";
            conn->send(response.dump());
        }else{  // 成功登录
            // 记录用户连接信息 注意线程安全！
            {
                lock_guard<mutex> lock(_connMutex); // 仅仅在该作用域有效，离开后自动析构解锁
                _userConnMap.insert({id,conn});
            }
  
            // 更新用户状态信息 state offline=>online
            user.setState("online");
            _userModel.updateState(user);

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();
            
            //查询该用户是否有离线消息
            vector<string> offstrs = _offlineMsgModel.query(id);
            if(offstrs.size() > 0){
                response["offlinemsg"] = offstrs;
                _offlineMsgModel.remove(id);
            }

            // 查询该用户的好友信息，并返回
            vector<User> friends = _friendModel.query(id);
            if (!friends.empty())
            {
                vector<string> vec2;
                for (User &f : friends)
                {
                    json js;
                    js["id"] = f.getId();
                    js["name"] = f.getName();
                    js["state"] = f.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }


            conn->send(response.dump());
        }
    }else{
        // 该用户不存在，用户存在但是密码错误，登录失败
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "id or password is invalid!";
        conn->send(response.dump());
    }
    
}
// 处理注册业务
void ChatService::reg(const TcpConnectionPtr & conn,json & js, Timestamp time)
{
    string name = js["name"];
    string pwd = js["password"];


    User user;
    user.setName(name);
    user.setPwd(pwd);
    bool state = _userModel.insert(user);
    if (state)
    {
        // 注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;  // 0 表示成功
        response["id"] = user.getId();
        conn->send(response.dump()); // 回复响应
    }
    else
    {
        // 注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;  // 1 注册失败错误
        conn->send(response.dump());
    }
}

// 处理客户端异常退出
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        lock_guard<mutex> lock(_connMutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
        {
            if (it->second == conn)
            {
                // 从map表删除用户的链接信息
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }

    // 用户注销，相当于就是下线，在redis中取消订阅通道
    // _redis.unsubscribe(user.getId()); 

    // 更新用户的状态信息
    if (user.getId() != -1)
    {
        user.setState("offline");
        _userModel.updateState(user);
    }
}

// 一对一聊天
void ChatService::oneToOneChat(const TcpConnectionPtr & conn,json & js, Timestamp time){
    int toid = js["to"].get<int>();

    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        if(it != _userConnMap.end()){ // 在线
            // toid
            it->second->send(js.dump());
            return ;
        }
    }

    // 不在线，存储在对方的离线消息中
    _offlineMsgModel.insert(toid,js.dump());
}

// 处理服务器异常退出
void ChatService::reset(){
    // 将所有在线的用户状态改为offline
    _userModel.resetState();
}


void ChatService::addFriend(const TcpConnectionPtr & conn,json & js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();
    // 储存好友信息
    _friendModel.insert(userid,friendid);

}

// 创建群组业务
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    // 存储新创建的群组信息
    Group group(-1, name, desc);
    if (_groupModel.createGroup(group))
    {
        // 存储群组创建人信息
        _groupModel.addGroup(userid, group.getId(), "creator");
    }
}
// 加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    _groupModel.addGroup(userid, groupid, "normal");
}
// 群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);

    lock_guard<mutex> lock(_connMutex);
    for (int id : useridVec)
    {
        auto it = _userConnMap.find(id);
        if (it != _userConnMap.end())
        {
            // 转发群消息
            it->second->send(js.dump());
        }
        else
        {
            // 查询toid是否在线 
            // User user = _userModel.query(id);
            // if (user.getState() == "online")
            // {
            //     _redis.publish(id, js.dump());
            // }
            // else
            {
                // 存储离线群消息
                _offlineMsgModel.insert(id, js.dump());
            }
        }
    }
}

// 处理注销业务
void ChatService::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();

    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if (it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }

    // 用户注销，相当于就是下线，在redis中取消订阅通道
    // _redis.unsubscribe(userid); 

    // 更新用户的状态信息
    User user(userid, "", "", "offline");
    _userModel.updateState(user);
}