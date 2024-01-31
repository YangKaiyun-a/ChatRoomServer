//
// Created by yky on 24-1-30.
//

#ifndef CHATROOMSERVER_DBMANAGER_H
#define CHATROOMSERVER_DBMANAGER_H

#include <sqlite3.h>
#include <string>
#include "global.h"

class DBManager {

public:
    DBManager();
    ~DBManager();
    static DBManager *instance();
    bool connectDataBase();
    //获取可执行文件的目录
    static std::string getExecutablePath();
    LoginResultType validateCredentials(const std::string &username, const std::string &password);
    LoginResultType signUpNewAccount(const std::string& username, const std::string& password);

private:
    sqlite3 *m_db{};
    static DBManager* m_dbManager;
    std::string m_dbPath;
};


#endif //CHATROOMSERVER_DBMANAGER_H
