#include "json.hpp"
#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>

using namespace std;
using json = nlohmann::json;

#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "group.hpp"
#include "user.hpp"
#include "public.hpp"

// 记录当前登录用户的信息
User g_currentUser;
// 记录当前登录用户好友列表信息
vector<User> g_currentUserFriendList;
// 记录当前登录用户所属群组列表信息
vector<Group> g_currentUserGroupList;
// 控制主菜单页面程序
bool isMainMenuRunning = false;
// 显示当前登录成功用户的基本信息
void showCurrentUserData();

// 接收线程
void readTaskHandler(int clientfd);

// 获取系统时间（聊天添加时间信息）
string getCurrentTime();

// 主聊天页面程序
void mainMenu(int clientfd);

// main函数用作发送线程
int main(int argc, char **argv)
{
    if (argc < 3)
    {
        cerr << "command invalid! example: .ChatClient 127.0.0.1 6000 " << endl;
        exit(-1);
    }
    // 解析通过命令行参数传递的ip和port
    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);

    // 创建client端的socket
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == clientfd)
    {
        cerr << "socket create error! " << endl;
        exit(-1);
    }

    // 填写client需要连接的sever信息ip+port
    sockaddr_in server;
    memset(&server, 0, sizeof(sockaddr_in));

    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(ip);

    // client和server进行连接
    if (-1 == connect(clientfd, (sockaddr *)&server, sizeof(sockaddr_in)))
    {
        cerr << "connect sever error" << endl;
        close(clientfd);
        exit(-1);
    }

    // main线程用于接收用户输入，用于发数据
    for (;;)
    {
        // 显示首页页面菜单 登录、注册、退出
        cout << "=======================================" << endl;
        cout << "1.login" << endl;
        cout << "2.register" << endl;
        cout << "3.quit" << endl;
        cout << "=======================================" << endl;
        cout << "choice:" << endl;
        int choice = 0;
        cin >> choice;
        cin.get(); // 读取缓冲区残留的回车

        switch (choice)
        {
        case 1: // login业务
        {
            int id = 0;
            char pwd[50] = {0};
            cout << "userid:" << endl;
            cin >> id;
            cin.get(); // 读取缓冲区残留的回车
            cout << "userpassword:" << endl;
            cin.getline(pwd, 50);

            json js;
            js["msgid"] = LOGIN_MSG;
            js["id"] = id;
            js["password"] = pwd;
            string request = js.dump();

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send login msg error: " << request << endl;
            }
            else
            {
                char buffer[1024] = {0};
                len = recv(clientfd, buffer, 1024, 0);
                if (-1 == len)
                {
                    cerr << "recv login response error" << endl;
                }
                else
                {
                    json responsejs = json::parse(buffer);
                    if (0 != responsejs["errno"].get<int>()) // 登录失败
                    {
                        cerr << responsejs["errmsg"] << endl;
                    }
                    else // 登录成功
                    {
                        // 初始化好友和群组列表
                        g_currentUserFriendList.clear();
                        g_currentUserGroupList.clear();
                        // 记录当前用户的id和name
                        g_currentUser.setId(responsejs["id"].get<int>());
                        g_currentUser.setName(responsejs["name"]);

                        // 记录好友列表信息
                        if (responsejs.contains("friends"))
                        {
                            vector<string> vec = responsejs["friends"];
                            for (string &str : vec)
                            {
                                json js = json::parse(str);
                                User user;
                                user.setId(js["id"]);
                                user.setName(js["name"]);
                                user.setState(js["state"]);
                                g_currentUserFriendList.push_back(user);
                            }
                        }

                        // 记录当前用户所属群组信息
                        if (responsejs.contains("groups"))
                        {
                            vector<string> vec1 = responsejs["groups"];
                            for (string &groupstr : vec1)
                            {
                                json grpjs = json::parse(groupstr);
                                Group group;
                                group.setId(grpjs["id"].get<int>());
                                group.setName(grpjs["groupname"]);
                                group.setDesc(grpjs["groupdesc"]);

                                vector<string> vec2 = grpjs["users"];
                                for (string &userstr : vec2)
                                {
                                    GroupUser user;
                                    json js = json::parse(userstr);
                                    user.setId(js["id"]);
                                    user.setName(js["name"]);
                                    user.setState(js["state"]);
                                    user.setRole(js["role"]);
                                    group.getUsers().push_back(user);
                                }
                                g_currentUserGroupList.push_back(group);
                            }
                        }

                        // 显示登录用户的基本信息
                        showCurrentUserData();

                        // 显示当前用户的离线消息
                        if (responsejs.contains("offlinemsg"))
                        {
                            vector<string> vec = responsejs["offlinemsg"];
                            for (string &str : vec)
                            {
                                json js = json::parse(str);
                                // time id name said : xxxxxxxxxxx
                                int msgType = js["msgid"].get<int>();
                                if (ONE_CHAT == msgType)
                                {
                                    cout << js["time"].get<string>() << " [" << js["id"].get<int>() << "]" << js["name"].get<string>() << " said "
                                         << js["msg"].get<string>() << endl;
                                    continue;
                                }
                                if (GROUP_CHAT_MSG == msgType)
                                {
                                    cout << "群消息" << "[" << js["groupid"] << "]: " << js["time"].get<string>() << " [" << js["id"].get<int>() << "]" << js["name"].get<string>() << " said " << js["msg"].get<string>() << endl;
                                    continue;
                                }
                            }
                        }

                        // 启动接收线程负责接收数据
                        static int threadNum = 0;
                        if (threadNum == 0)
                        {
                            std::thread readTask(readTaskHandler, clientfd);
                            readTask.detach();
                            threadNum++;
                        }

                        // 进入主页面
                        isMainMenuRunning = true;
                        mainMenu(clientfd);
                    }
                }
            }
            break;
        }
        case 2: // 注册业务
        {
            char name[50] = {0};
            char pwd[50] = {0};
            cout << "username:";
            cin.getline(name, 50);
            cout << "password:";
            cin.getline(pwd, 50);

            json js;
            js["msgid"] = REG_MSG;
            js["name"] = name;
            js["password"] = pwd;
            string request = js.dump();

            int len = send(clientfd, request.c_str(), strlen(request.c_str()) + 1, 0);
            if (len == -1)
            {
                cerr << "send reg msg error: " << request << endl;
            }
            else
            {
                char buffer[1024] = {0};
                len = recv(clientfd, buffer, 1024, 0);
                if (-1 == len)
                {
                    cerr << "recv reg response error" << endl;
                }
                else
                {
                    json responsejs = json::parse(buffer);
                    if (0 != responsejs["errno"].get<int>()) // 注册失败
                    {
                        cerr << name << " is already exist, register error!" << endl;
                    }
                    else // 注册成功
                    {
                        cout << name << " register success, user id is " << responsejs["id"] << ", do not foget it!";
                    }
                }
            }
        }
        break;
        case 3: // 退出业务
        {
            close(clientfd);
            exit(0);
        }
        default:
        {
            cerr << "invalid input!" << endl;
            break;
        }
        }
    }
}

// 显示当前登录成功用户的基本信息
void showCurrentUserData()
{
    cout << "=========================login user===========================" << endl;
    cout << "current login user => id:" << " [" << g_currentUser.getId() << "]" << "name:" << g_currentUser.getName() << endl;
    cout << "-------------------------friend list--------------------------" << endl;
    if (!g_currentUserFriendList.empty())
    {
        for (User &user : g_currentUserFriendList)
        {
            cout << user.getId() << " " << user.getName() << " " << user.getState() << endl;
        }
    }
    cout << "-------------------------group list--------------------------" << endl;
    if (!g_currentUserGroupList.empty())
    {
        for (Group &group : g_currentUserGroupList)
        {
            cout << group.getId() << " " << group.getName() << " " << group.getDesc() << endl;
            for (GroupUser &groupUser : group.getUsers())
            {
                cout << groupUser.getId() << " " << groupUser.getName() << " " << groupUser.getState() << " " << groupUser.getRole() << endl;
            }
        }
    }
    cout << "============================================================" << endl;
}

void readTaskHandler(int clientfd)
{
    for (;;)
    {
        char buffer[1024] = {0};
        int len = recv(clientfd, buffer, 1024, 0);
        if (-1 == len || 0 == len)
        {
            close(clientfd);
            exit(-1);
        }
        json js = json::parse(buffer);
        int msgType = js["msgid"].get<int>();
        if (ONE_CHAT == msgType)
        {
            cout << js["time"].get<string>() << " [" << js["id"].get<int>() << "]" << js["name"].get<string>() << " said "
                 << js["msg"].get<string>() << endl;
            continue;
        }
        if (GROUP_CHAT_MSG == msgType)
        {
            cout << "群消息" << "[" << js["groupid"] << "]: " << js["time"].get<string>() << " [" << js["id"].get<int>() << "]" << js["name"].get<string>() << " said " << js["msg"].get<string>() << endl;
            continue;
        }
    }
}

void help(int clientfd = 0, string str = "");
void chat(int clientfd, string str);
void addfriend(int clientfd, string str);
void creategroup(int clientfd, string str);
void addgroup(int clientfd, string str);
void groupchat(int clientfd, string str);
void logout(int clientfd, string str);

unordered_map<string, string> commandMap = {
    {"help", "显示支持的命令"},
    {"chat", "一对一聊天，格式chat:friendid:message"},
    {"addfriend", "添加好友，格式addfriend:friendid"},
    {"creategroup", "创建群组，格式creategroup:groupname:groupdesc"},
    {"addgroup", "加入群组，格式addgroup:groupid"},
    {"groupchat", "群聊，格式groupchat:groupid:message"},
    {"logout", "注销，格式logout"}};

unordered_map<string, function<void(int, string)>> commandHandlerMap = {
    {"help", help},
    {"chat", chat},
    {"addfriend", addfriend},
    {"creategroup", creategroup},
    {"addgroup", addgroup},
    {"groupchat", groupchat},
    {"logout", logout}};

void mainMenu(int clientfd)
{
    help();

    char buffer[1024] = {0};
    for (; isMainMenuRunning;)
    {
        cin.getline(buffer, 1024);
        string commandbuf(buffer);
        string command;
        int idx = commandbuf.find(":");
        if (-1 == idx)
        {
            command = commandbuf;
        }
        else
        {
            command = commandbuf.substr(0, idx);
        }
        auto it = commandHandlerMap.find(command);
        if (it == commandHandlerMap.end())
        {
            cerr << "invalid input command!" << endl;
            continue;
        }

        // 调用相应函数
        it->second(clientfd, commandbuf.substr(idx + 1, commandbuf.size() - idx - 1));
    }
}

void help(int clientfd, string str)
{
    cout << "show command list >>>" << endl;
    for (auto &p : commandMap)
    {
        cout << p.first << " :" << p.second << endl;
    }
    cout << endl;
}

// addfriend command handler
void addfriend(int clientfd, string str)
{
    int friendid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_FRIEND_MSG;
    js["id"] = g_currentUser.getId();
    js["friendid"] = friendid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send addfriend msg error ->" << buffer << endl;
    }
    else
    {
        cerr << "添加成功" << endl;
    }
}

// chat command handler
void chat(int clientfd, string str)
{
    int idx = str.find(":");
    if (-1 == idx)
    {
        cerr << "chat command invalid!" << endl;
        return;
    }
    int friendid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx - 1);

    json js;
    js["msgid"] = ONE_CHAT;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["toid"] = friendid;
    js["time"] = getCurrentTime();
    js["msg"] = message;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send chat msg error ->" << buffer << endl;
    }
}

void creategroup(int clientfd, string str)
{
    int idx = str.find(":");
    if (idx == -1)
    {
        cerr << "create group command invalid";
        return;
    }

    string groupname = str.substr(0, idx);
    string groupdesc = str.substr(idx + 1, str.size() - idx - 1);

    json js;
    js["msgid"] = CREATE_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupname"] = groupname;
    js["groupdesc"] = groupdesc;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "create group fail ->" << endl;
    }
    else
    {
        cerr << "create group success ->" << endl;
    }
}
void addgroup(int clientfd, string str)
{
    int groupid = atoi(str.c_str());
    json js;
    js["msgid"] = ADD_GROUP_MSG;
    js["id"] = g_currentUser.getId();
    js["groupid"] = groupid;
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send addgroupid msg error ->" << buffer << endl;
    }
    else
    {
        cerr << "添加成功" << endl;
    }
}
void groupchat(int clientfd, string str)
{
    int idx = str.find(":");
    if (idx == -1)
    {
        cerr << "groupchat command invalid";
        return;
    }

    int groupid = atoi(str.substr(0, idx).c_str());
    string message = str.substr(idx + 1, str.size() - idx - 1);

    json js;
    js["msgid"] = GROUP_CHAT_MSG;
    js["id"] = g_currentUser.getId();
    js["name"] = g_currentUser.getName();
    js["groupid"] = groupid;
    js["msg"] = message;
    js["time"] = getCurrentTime();

    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "发送失败 ->" << endl;
    }
}
void logout(int clientfd, string str)
{
    json js;
    js["msgid"] = LOGOUT_MSG;
    js["id"] = g_currentUser.getId();
    string buffer = js.dump();

    int len = send(clientfd, buffer.c_str(), strlen(buffer.c_str()) + 1, 0);
    if (-1 == len)
    {
        cerr << "send logout msg error ->" << buffer << endl;
    }
    else
    {
        isMainMenuRunning = false;
    }
}

string getCurrentTime()
{
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);

    std::tm *local_tm = std::localtime(&time_t_now);

    std::ostringstream oss;
    oss << std::put_time(local_tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}