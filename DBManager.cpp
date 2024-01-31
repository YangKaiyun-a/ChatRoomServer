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
const char* createTableSQL =
        "CREATE TABLE IF NOT EXISTS AccountTable ("
        "Account TEXT PRIMARY KEY,"
        "Password TEXT);";

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

    char* errMsg = nullptr;
    rc = sqlite3_exec(m_db, createTableSQL, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK)
    {
        std::cerr << "SQL error: " << errMsg << std::endl;
        sqlite3_free(errMsg);
        sqlite3_close(m_db);
        return false;
    }

    return true;
}

std::string DBManager::getExecutablePath()
{
    char path[PATH_MAX];
    memset(path, 0, sizeof(path));
    readlink("/proc/self/exe", path, sizeof(path) - 1);
    std::string exePath(path);
    return exePath.substr(0, exePath.find_last_of('/'));
}

LoginResultType DBManager::validateCredentials(const std::string &username, const std::string &password)
{
    sqlite3_stmt* stmt;
    std::string query = "SELECT Password FROM " + DATABASETABLE_NAME + " WHERE Account = ?";

    int rc = sqlite3_prepare_v2(m_db, query.c_str(), -1, &stmt, NULL);
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
            return Login_Successed;
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

LoginResultType DBManager::signUpNewAccount(const std::string& username, const std::string& password)
{
    sqlite3_stmt* stmt;
    std::string checkUserQuery = "SELECT * FROM " + DATABASETABLE_NAME + " WHERE Account = ?";

    int rc = sqlite3_prepare_v2(m_db, checkUserQuery.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK)
    {
        std::cerr << "Failed to prepare statement" << std::endl;
        return Erro_Sql;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);

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
    rc = sqlite3_prepare_v2(m_db, insertUserQuery.c_str(), -1, &stmt, NULL);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement" << std::endl;
        return Erro_Sql;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, password.c_str(), -1, SQLITE_STATIC);

    rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "Insert query execution failed" << std::endl;
        sqlite3_finalize(stmt);
        return Erro_Sql;
    }

    sqlite3_finalize(stmt);
    return SignUp_Successed;
}