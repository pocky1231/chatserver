#include "redis.hpp"
#include <iostream>

using namespace std;

Redis::Redis()
    : _publish_context(nullptr), _subscribe_context(nullptr)
{
}

Redis::~Redis()
{
    if (_publish_context != nullptr)
    {
        redisFree(_publish_context);
    }
    if (_subscribe_context != nullptr)
    {
        redisFree(_subscribe_context);
    }
}

// 连接redis服务器
bool Redis::connect()
{
    //负责publish上下文连接
    _publish_context = redisConnect("127.0.0.1",6379);
    if(nullptr == _publish_context)
    {
        cerr << "connected redis failed!" << endl;
        return false;
    }
    //负责subscribe上下文连接
    _subscribe_context = redisConnect("127.0.0.1",6379);
    if(nullptr == _subscribe_context)
    {
        cerr << "connected redis failed!" << endl;
        return false;
    }

    thread t([&](){
        observer_channel_message();
    });
    t.detach();

    cout << "connect redis-server success!" << endl;
    return true;
}

// 向redis指定的channel发布消息
bool Redis::publish(int channel, string message)
{
    redisReply *reply = (redisReply *) redisCommand(_publish_context, "PUBLISH %d %s", channel, message.c_str());
    if(nullptr == reply)
    {
        cerr << "publish command failed!" <<endl;
        return false;
    }
    freeReplyObject(reply);
    return true;
}
// 向redis指定的channel订阅消息
bool Redis::subscribe(int channel)
{
    // SUBSCRIBE命令本身会造成线程阻塞等待通道里面发生消息，这里只订阅通道，不阻塞接收消息
    // 接收消息专门在observer_channel_message函数中的独立线程中实现
    if(REDIS_ERR == redisAppendCommand(this->_subscribe_context, "SUBSCRIBE %d", channel))
    {
        cerr << "subscribe command failed!" << endl;
        return false;
    }
    int done = 0;
    while(!done)
    {
        if(REDIS_ERR == redisBufferWrite(this->_subscribe_context, &done))
        {
            cerr << "subscribe command failed!" <<endl;
            return false;
        }
    }

    return true;
}
// 向redis指定的channel取消订阅
bool Redis::unsubscribe(int channel)
{
    // UNSUBSCRIBE命令本身会造成线程阻塞等待通道里面发生消息，这里只订阅通道，不阻塞接收消息
    if(REDIS_ERR == redisAppendCommand(this->_subscribe_context, "UNSUBSCRIBE %d", channel))
    {
        cerr << "subscribe command failed!" << endl;
        return false;
    }
    int done = 0;
    while(!done)
    {
        if(REDIS_ERR == redisBufferWrite(this->_subscribe_context, &done))
        {
            cerr << "subscribe command failed!" <<endl;
            return false;
        }
    }

    return true;
}
// 在独立线程中接收订阅通道消息
void Redis::observer_channel_message()
{
    redisReply *reply =nullptr;
    while(REDIS_OK==redisGetReply(this->_subscribe_context,(void **)&reply))
    {
        if(reply != nullptr && reply->element[2] != nullptr && reply->element[2]->str != nullptr)
        {
            _notify_message_handler(atoi(reply->element[1]->str), reply->element[2]->str);
        }
    }
}
// 初始化向业务层上通报消息的回调对象
void Redis::init_notify_handler(function<void(int, string)> fn)
{
    this->_notify_message_handler = fn;
}