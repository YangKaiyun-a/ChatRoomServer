#include <iostream>

#include "Server.h"
#include "DBManager.h"


int main()
{
    //启动数据库
    if(!DBManager::instance()->connectDataBase())
        return 0;

    Server server(1234);
    server.start();
    return 0;
}
