//
// Created by yky on 24-1-30.
//

#include "Server.h"
#include "global.h"
#include "DBManager.h"

#include <iostream>
#include <sys/types.h>
#include <cstring>
#include <json/json.h>
#include <arpa/inet.h>
#include <thread>


Server::Server(int port)
{
    //sockfd用来监听客户端请求
    m_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (m_sockfd < 0)
    {
        error("ERROR opening socket");
    }

    int opt = 1;
    if(setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        error("ERROR on setting SO_REUSEADDR");
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    m_port = port;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(m_port);

    if (bind(m_sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0)
    {
        error("ERROR on binding");
    }
}

Server::~Server()
{
    close(m_sockfd);
}

void Server::start()
{
    listen(m_sockfd, 5);
    clilen = sizeof(cli_addr);

    while (true)
    {
        //每个ip只创建一个
        m_newsockfd = accept(m_sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if (m_newsockfd < 0)
        {
            error("ERROR on accept");
            continue;
        }

        char *ip = inet_ntoa(cli_addr.sin_addr);
        std::cout << "Client connected from " << ip << std::endl;
        m_clientSockets.push_back(m_newsockfd);

        //创建一个新线程处理这个客户端
        std::thread clientThread(&Server::handleClient, this, m_newsockfd, ip);
        clientThread.detach();
    }
}

//登录请求返回
void Server::handleLogin(int sockfd, LoginResultType type)
{
    Json::Value root;
    root["type"] = LoginResult;

    Json::Value messages;
    messages["code"] = type;

    root["messages"] = messages;

    //将JSON对象转换为字符串
    Json::StreamWriterBuilder builder;
    std::string jsonString = Json::writeString(builder, root);

    ssize_t bytesSent = write(sockfd, jsonString.c_str(), jsonString.length());
    if (bytesSent < 0)
    {
        // 发送失败，处理错误
        perror("Error sending message");
    }
}

//注册请求返回
void Server::handleSignUp(int sockfd, LoginResultType type)
{
    Json::Value root;
    root["type"] = SignupResult;

    Json::Value messages;
    messages["code"] = type;

    root["messages"] = messages;

    //将JSON对象转换为字符串
    Json::StreamWriterBuilder builder;
    std::string jsonString = Json::writeString(builder, root);

    ssize_t bytesSent = write(sockfd, jsonString.c_str(), jsonString.length());
    if (bytesSent < 0)
    {
        // 发送失败，处理错误
        perror("Error sending message");
    }
}

void Server::handleClient(int clientSockfd, char *ip)
{
    while(true)
    {
        //存储接收到的消息
        char buffer[256];
        bzero(buffer, 256);
        int n = read(clientSockfd, buffer, 255);
        if(n < 0)
        {
            perror("ERROR reading from socket");
            std::cerr << "Client :" << ip << "  Client disconnected." << std::endl;
            break;
        }
        else if(n == 0)
        {
            //客户端关闭了连接
            std::cout << "Client disconnected." << std::endl;
            break;
        }

        //解析为json格式
        Json::Value root;
        Json::Reader reader;
        if(reader.parse(buffer, root))
        {
            int messageType = root["type"].asInt();
            std::string userName = root["messages"]["username"].asString();
            std::string passWord = root["messages"]["password"].asString();

            switch (messageType)
            {
                case Login:
                {
                    //处理登录
                    handleLogin(clientSockfd, DBManager::instance()->validateCredentials(userName, passWord));
                    break;
                }
                case SignUp:
                {
                    //处理注册
                    handleSignUp(clientSockfd, DBManager::instance()->signUpNewAccount(userName, passWord));
                    break;
                }
                case Normal:
                {
                    //正常信息，转发给所有客户端
                    for(int sockfd : m_clientSockets)
                    {
                        write(sockfd, buffer, n);
                    }
                    break;
                }
                default:
                    break;
            }
        }
        else
        {
            std::cerr << "Failed to parse Json:" << reader.getFormattedErrorMessages();
        }
    }
    close(clientSockfd);
}
