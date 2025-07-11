#ifndef PUBLIC_H
#define PUBLIC_H

// server和service的公共文件

enum EnMsgType
{
    LOGIN_MSG = 1, //登录消息
    REG_MSG = 2,    //注册消息
    REG_MSG_ACK, //注册响应
    LOGIN_MSG_ACK, //登录响应

    ONE_CHAT, //一对一聊天
    ADD_FRIEND_MSG, //添加好友

    CREATE_GROUP_MSG, //创建群组
    ADD_GROUP_MSG,    //加入群组
    GROUP_CHAT_MSG,    //群聊天
    LOGOUT_MSG


};


#endif