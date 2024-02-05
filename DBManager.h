//
// Created by yky on 24-1-30.
//

#ifndef CHATROOMSERVER_DBMANAGER_H
#define CHATROOMSERVER_DBMANAGER_H

#include <sqlite3.h>
#include <string>
#include <vector>
#include "global.h"

class DBManager {

public:
    DBManager();
    ~DBManager();
    static DBManager *instance();
    bool connectDataBase();
    //获取可执行文件的目录
    static std::string getExecutablePath();
    LoginResultType validateCredentials(const std::string &userName, const std::string &password);
    LoginResultType signUpNewAccount(const std::string& userName, const std::string& password);
    //添加一条记录到已登录用户表
    LoginResultType addLoggedInUser(const std::string& userName);
    //删除一条记录到已登录用户表
    void removeLoggedInUser(const std::string& userName);
    //获取已登录用户表中的用户名
    std::vector<std::string> getLoggedInUserAllRecords();
    //添加一条记录到聊天记录表
    bool addSingleChatHistory(const std::string& userName, const std::string& message, const std::string& timeStamp);
    //获取一条聊天记录
    std::vector<std::string> getRecordsFormChatHistory(int count);

private:
    sqlite3 *m_db{};
    static DBManager* m_dbManager;
    std::string m_dbPath;
};


#endif //CHATROOMSERVER_DBMANAGER_H
