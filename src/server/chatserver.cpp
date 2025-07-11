#include "chatserver.hpp"
#include "json.hpp"
#include "chatservice.hpp"

#include <iostream>
#include <functional>
#include <string>
using namespace std;
using namespace placeholders;
using json = nlohmann::json;

ChatServer::ChatServer(EventLoop *loop, const InetAddress &listenAddr, const string &nameArg) : _server(loop, listenAddr, nameArg), _loop(loop)
{
    // 注册连接回调
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));

    // 注册消息回调
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));

    // 设置线程数量
    _server.setThreadNum(4);
}

// 启动服务
void ChatServer::start()
{
    _server.start();
}

// 上报连接相关信息的回调函数
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    // 用户断开连接
    if (!conn->connected())
    {
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}

// 上报读写相关信息的回调函数
void ChatServer::onMessage(const TcpConnectionPtr &conn, Buffer *buffer, Timestamp time)
{
    string buf = buffer->retrieveAllAsString();
    /* 粗判是否为 HTTP 报文 */
    if (buf.substr(0, 3) == "GET" || buf.substr(0, 4) == "POST" ||
        buf.substr(0, 4) == "HEAD" || buf.substr(0, 3) == "PUT")
    {
        cout << "Non‑JSON request, send 400" << endl;
        conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");
        return;
    }

    try
    {
        // 数据反序列化
        json js = json::parse(buf);
        // 完全解耦网络模块的代码和业务模块的代码
        // 通过js["msgid"] 获取=》业务handler=》conn js time
        auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
        msgHandler(conn, js, time);
    }
    catch (const nlohmann::json::parse_error &e)
    {
        cout << "Bad JSON: " << e.what() << " | raw: " << buf;
        conn->shutdown();
    }
}