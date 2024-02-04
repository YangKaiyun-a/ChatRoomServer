//
// Created by yky on 24-1-30.
//

#include "DBManager.h"
#include "iostream"
#include <unistd.h>
#include <linux/limits.h>
#include <cstring>
#include <filesystem>

namespace fs = std::filesystem;

const std::string DATABASE_NAME = "userdatabase.db3";
const std::string DATABASETABLE_NAME = "AccountTable";

//创建用户表
const char* createTableSQL =
        "CREATE TABLE IF NOT EXISTS AccountTable ("
        "Account TEXT PRIMARY KEY,"
        "Password TEXT);";

//创建已登录用户表
const char* createLoggedInUserTableSQL =
        "CREATE TABLE IF NOT EXISTS LoggedInUsers ("
        "username TEXT PRIMARY KEY);";

DBManager *DBManager::m_dbManager =  nullptr;

DBManager::DBManager()
{
    m_dbPath = getExecutablePath() + "/db/" + DATABASE_NAME;
}

DBManager::~DBManager()
{
    sqlite3_close(m_db);
}

DBManager *DBManager::instance()
{
    if(nullptr == m_dbManager)
    {
        m_dbManager = new DBManager();
    }

    return m_dbManager;
}

bool DBManager::connectDataBase()
{
    //创建数据库目录（如果不存在）
    std::string dbDir = m_dbPath.substr(0, m_dbPath.find_last_of('/'));
    if(!dbDir.empty() && !fs::exists(dbDir))
    {
        fs::create_directories(dbDir);
    }

    //打开数据库
    int rc = sqlite3_open(m_dbPath.c_str(), &m_db);
    if(rc)
    {
        std::cerr << "Can't open database: " << sqlite3_errmsg(m_db);
        sqlite3_close(m_db);
        return false;
    }
    else
    {
        std::cout << "Open database succeed!" << std::endl;
    }

    //创建用户表（如果不存在）
    char* errMsg = nullptr;
    rc = sqlite3_exec(m_db, createTableSQL, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK)
    {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        sqlite3_close(m_db);
        return false;
    }

    //创建已登录用户表（如果不存在）
    rc = sqlite3_exec(m_db, createLoggedInUserTableSQL, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK)
    {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        sqlite3_close(m_db);
        return false;
    }

    return true;
}

//获取可执行文件目录路径
std::string DBManager::getExecutablePath()
{
    char path[PATH_MAX];
    memset(path, 0, sizeof(path));
    readlink("/proc/self/exe", path, sizeof(path) - 1);
    std::string exePath(path);
    return exePath.substr(0, exePath.find_last_of('/'));
}

//验证密码
LoginResultType DBManager::validateCredentials(const std::string &username, const std::string &password)
{
    sqlite3_stmt* stmt;
    std::string query = "SELECT Password FROM " + DATABASETABLE_NAME + " WHERE Account = ?";

    int rc = sqlite3_prepare_v2(m_db, query.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        std::cerr << "Failed to prepare statement" << std::endl;
        return Erro_Sql;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
    {
        const unsigned char* retrievedPassword = sqlite3_column_text(stmt, 0);
        if (password == reinterpret_cast<const char*>(retrievedPassword))
        {
            sqlite3_finalize(stmt);

            //在已登录用户表中添加一个用户
            return addLoggedInUser(username);
        }
        else
        {
            sqlite3_finalize(stmt);
            return Error_Password;
        }
    }

    if (rc != SQLITE_DONE)
    {
        std::cerr << "Query execution failed" << std::endl;
        sqlite3_finalize(stmt);
        return Erro_Sql;
    }

    sqlite3_finalize(stmt);
    return Unknow_UserName;
}

//添加新用户
LoginResultType DBManager::signUpNewAccount(const std::string& username, const std::string& password)
{
    //stmt用于指向预备语句对象
    sqlite3_stmt* stmt;
    std::string checkUserQuery = "SELECT * FROM " + DATABASETABLE_NAME + " WHERE Account = ?";

    //编译预备语句
    int rc = sqlite3_prepare_v2(m_db, checkUserQuery.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        std::cerr << "Failed to prepare statement" << std::endl;
        return Erro_Sql;
    }

    //绑定查询参数
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

    //执行语句
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        sqlite3_finalize(stmt);
        return Exist_UserName;
    }
    else if (rc != SQLITE_DONE)
    {
        std::cerr << "Query execution failed" << std::endl;
        sqlite3_finalize(stmt);
        return Erro_Sql;
    }

    sqlite3_finalize(stmt);

    std::string insertUserQuery = "INSERT INTO " + DATABASETABLE_NAME + " (Account, Password) VALUES (?, ?)";
    rc = sqlite3_prepare_v2(m_db, insertUserQuery.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        std::cerr << "Failed to prepare statement" << std::endl;
        return Erro_Sql;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
    {
        std::cerr << "Insert query execution failed" << std::endl;
        sqlite3_finalize(stmt);
        return Erro_Sql;
    }

    sqlite3_finalize(stmt);
    return SignUp_Successed;
}

//添加一条记录到已登录用户表
LoginResultType DBManager::addLoggedInUser(const std::string &username)
{
    //检查用户是否已登录
    std::string sqlCheck = "SELECT * FROM LoggedInUsers WHERE username = ?;";
    sqlite3_stmt* stmt;
    int rc = sqlite3_prepare_v2(m_db, sqlCheck.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        std::cerr << "Failed to prepare statement" << std::endl;
        return Erro_Sql;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW)
    {
        // 用户已登录
        sqlite3_finalize(stmt);
        return AlreadyLoggedIn;
    }
    sqlite3_finalize(stmt);

    //若未登录则添加新记录
    std::string sqlInsert = "INSERT INTO LoggedInUsers (username) VALUES (?);";

    rc = sqlite3_prepare_v2(m_db, sqlInsert.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        std::cerr << "Failed to prepare insert statement" << std::endl;
        return Erro_Sql;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
    {
        std::cerr << "Insert query execution failed" << std::endl;
        sqlite3_finalize(stmt);
        return Erro_Sql;
    }

    sqlite3_finalize(stmt);

}

void DBManager::removeLoggedInUser(const std::string &username)
{
    const std::string sql = "DELETE FROM LoggedInUsers WHERE username = ?;";
    sqlite3_stmt* stmt;

    int rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK)
    {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(m_db) << std::endl;
        return;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE)
    {
        std::cerr << "Failed to execute delete statement: " << sqlite3_errmsg(m_db) << std::endl;
    }

    sqlite3_finalize(stmt);
}

//获取已登录用户表中的用户名
std::vector<std::string> DBManager::getLoggedInUserAllRecords()
{
    std::vector<std::string> userNameVector;
    sqlite3_stmt* stmt;
    const std::string sql = "SELECT username FROM LoggedInUsers;";

    int rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, NULL);
    if(rc != SQLITE_OK)
    {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(m_db) << std::endl;
        return userNameVector;
    }

    while(sqlite3_step(stmt) == SQLITE_ROW)
    {
        userNameVector.emplace_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
    }

    sqlite3_finalize(stmt);

    return userNameVector;
}
