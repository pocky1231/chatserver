#ifndef CHATSERVICE_H
#define CHATSERVICE_H

#include "json.hpp"
#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include "usermodel.hpp"
#include <mutex>
#include "offlinemessagemodel.hpp"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include "redis.hpp"
using namespace std;
using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;

using MsgHandler = std::function<void(const TcpConnectionPtr &conn, json &js, Timestamp time)>;

//聊天服务器业务类
class ChatService
{
public:
    //获取单例对象的接口函数
    static ChatService* instance();
    //处理登录业务
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //处理注册业务
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //处理一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //添加好友
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //获取消息对应的处理器
    MsgHandler getHandler(int msgid);
    //客户端异常退出处理
    void clientCloseException(const TcpConnectionPtr& cnn);
    //服务器异常业务重置方法 
    void reset();
    //创建群组
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //添加群组
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //群聊天
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //注销
    void logout(const TcpConnectionPtr &conn, json &js, Timestamp time);
    //redis订阅消息回调函数
    void handleRedisSubscribeMessage(int userid, string message);

private:
    //单例，构造函数私有化
    ChatService();

    //存储消息id和对应的业务处理
    unordered_map<int, MsgHandler> _msgHandlerMap;

    //存储在线用户的连接,注意线程安全
    unordered_map<int, TcpConnectionPtr> _userConnMap;

    //定义互斥锁，保证_unordered_map的线程安全
    mutex _connMutex;

    //数据操作对象类
    UserModel _userModel;

    //离线消息对象类
    OfflineMsgModel _offlineMsgModel;

    //添加好友对象类
    FriendModel _friendModel;

    //群聊类
    GroupModel _groupModel;

    //redis对象
    Redis _redis;




};






#endif