#ifndef OFFLINEMESSAGEMODEL_H
#define OFFLINEMESSAGEMODEL_H

#include<vector>
#include<string>
using namespace std;

//提供离线消息表的操作接口方法
class OfflineMsgModel
{
public:
    //存储离线消息
    void insert(int userid, string msg);

    //删除离线消息
    void remove(int userid);

    //查询离线消息
    vector<string> query(int userid);
private:
};





#endif