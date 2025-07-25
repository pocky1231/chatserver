#include "offlinemessagemodel.hpp"
#include <db.hpp>

// 存储离线消息
void OfflineMsgModel::insert(int userid, string msg)
{
   // 组装sql语句
   char sql[1024] = {0};
   sprintf(sql, "insert into offlinemessage(userid, message) values(%d, '%s')", userid, msg.c_str());

   MySQL mysql;
   if (mysql.connect())
   {
      mysql.update(sql);
   }
}

// 删除离线消息
void OfflineMsgModel::remove(int userid)
{
   // 组装sql语句
   char sql[1024] = {0};
   sprintf(sql, "delete from offlinemessage where userid = %d", userid);

   MySQL mysql;
   if (mysql.connect())
   {
      mysql.update(sql);
   }
}

// 查询离线消息
vector<string> OfflineMsgModel::query(int userid)
{
   // 组装sql语句
   char sql[1024] = {0};
   sprintf(sql, "select message from offlinemessage where userid = %d", userid);
   vector<string> vec;

   MySQL mysql;
   if (mysql.connect())
   {
      MYSQL_RES *res = mysql.query(sql);
      if (res != nullptr)
      {
         MYSQL_ROW row;
         // 把多条userid用户的所有离线消息放入vector返回
         while ((row = mysql_fetch_row(res)) != nullptr)
         {
            vec.push_back(row[0]);
         }
         mysql_free_result(res);
      }
   }
   return vec;
}