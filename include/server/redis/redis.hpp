#ifndef REDIS_H
#define REDIS_H

#include <hiredis/hiredis.h>
#include <thread>
#include <functional>
using namespace std;

class Redis
{
public:
    Redis();
    ~Redis();

    // 连接redis服务器
    bool connect();

    // 向redis指定的channel发布消息
    bool publish(int channel, string message);
    // 向redis指定的channel订阅消息
    bool subscribe(int channel);
    // 向redis指定的channel取消订阅
    bool unsubscribe(int channel);
    // 在独立线程中接收订阅通道消息
    void observer_channel_message();
    // 初始化向业务层上通报消息的回调对象
    void init_notify_handler(function<void(int, string)> fn);
    
private:
    // hiredis同步上下文对象，负责publish
    redisContext* _publish_context;
    // hiredis同步上下文对象，负责subscribe
    redisContext* _subscribe_context;
    // 回调操作，收到订阅的消息，给service层上报
    function<void(int, string)> _notify_message_handler;
};









#endif