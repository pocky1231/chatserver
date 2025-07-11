#include "groupmodel.hpp"
#include "db.hpp"

// 创建群组
bool GroupModel::createGroup(Group &group)
{
    // 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into allgroup(groupname, groupdesc) values('%s', '%s')", group.getName().c_str(), group.getDesc().c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        if (mysql.update(sql))
        {
            // 获取插入成功的用户数据生成的id
            group.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
    }
    return false;
}
// 加入群组
void GroupModel::addGroup(int userid, int groupid, string role)
{
    // 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into groupuser(userid, groupid, grouprole) values(%d, %d, '%s')", userid, groupid, role.c_str());

    MySQL mysql;
    if (mysql.connect())
    {
        mysql.update(sql);
    }
    return;
}
// 查询用户所在群组信息
vector<Group> GroupModel::queryGroups(int userid)
{
    char sql[1024] = {0};
    sprintf(sql, "select a.id, a.groupname, a.groupdesc from allgroup a inner join groupuser b on a.id = b.groupid where b.userid = %d", userid);

    vector<Group> groupVec;

    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            if ((row = mysql_fetch_row(res)) != nullptr)
            {
                Group group;
                group.setId(atoi(row[0]));
                group.setName(row[1]);
                group.setDesc(row[2]);
                groupVec.push_back(group);
            }
        }
        // 返回指针，内部申请了资源
        mysql_free_result(res);

        for (Group &group : groupVec)
        {
            sprintf(sql, "select a.id, a.name, a.state, b.grouprole from user a inner join groupuser b on a.id = b.userid where b.groupid = %d", group.getId());
            MYSQL_RES *res = mysql.query(sql);
            if (res != nullptr)
            {
                MYSQL_ROW row;
                while ((row = mysql_fetch_row(res)) != nullptr)
                {
                    GroupUser groupUser;
                    groupUser.setId(atoi(row[0]));
                    groupUser.setName(row[1]);
                    groupUser.setState(row[2]);
                    groupUser.setRole(row[3]);
                    group.getUsers().push_back(groupUser);                   
                } 
            }
            mysql_free_result(res);
        }
    }

    return groupVec;
}
// 根据groupid查询组用户id，除了userid自己
vector<int> GroupModel::queryGroupUsers(int userid, int groupid)
{
    char sql[1024] = {0};
    sprintf(sql, "select userid from groupuser where groupid = %d AND userid != %d", groupid, userid);
    vector<int> idVec;
    MySQL mysql;
    if (mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            if ((row = mysql_fetch_row(res)) != nullptr)
            {
                idVec.push_back(atoi(row[0]));
            }
        }
        // 返回指针，内部申请了资源
        mysql_free_result(res);
    }
    return idVec;
}