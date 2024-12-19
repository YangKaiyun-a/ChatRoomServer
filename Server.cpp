//
// Created by yky on 24-1-30.
//

#include "Server.h"
#include "global.h"
#include "DBManager.h"
#include "ThirdParty/json/json.h"

#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <thread>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>


Server::Server(int port)
{
    //m_sockfd用来监听客户端请求
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

[[noreturn]] void Server::start()
{
    listen(m_sockfd, 5);
    clilen = sizeof(cli_addr);

    while(true)
    {
        //每个ip只创建一个描述符
        int newSockfd = accept(m_sockfd, (struct sockaddr *) &cli_addr, &clilen);
        if(newSockfd < 0)
        {
            error("ERROR on accept");
            continue;
        }

        char *ip = inet_ntoa(cli_addr.sin_addr);
        std::cout << "Client connected from " << ip << std::endl;
        m_clientSockets.push_back(newSockfd);

        //创建一个新线程处理这个客户端
        std::thread clientThread(&Server::handleClient, this, newSockfd, ip);
        clientThread.detach();
    }
}

//处理登录请求
void Server::handleLogin(int sockfd, const std::string &userName, const std::string &password)
{
    //验证用户名和密码
    LoginResultType result = DBManager::instance()->validateCredentials(userName, password);

    //发送登录结果
    sendLoginResult(sockfd, result, userName);

    if(result == Login_Successed)
    {
        //发送当前在线用户给所有人
        sendOnlineUser();

        //发送之前的20条聊天记录给新登录用户
        sendLastedChatMessages(sockfd);
    }
}

//处理注册请求
void Server::handleSignUp(int sockfd, const std::string &userName, const std::string &password)
{
    //发送注册结果
    sandSingUpResult(sockfd, DBManager::instance()->signUpNewAccount(userName, password));
}

//在单独的线程中与每一个客户端进行通信
void Server::handleClient(int clientSockfd, char *ip)
{
    int circleCount = 1;
    std::string curUserName = "unknown_userName";

    while(true)
    {
        //每次接收32位的消息长度
        uint32_t messageLength;
        int n = read(clientSockfd, &messageLength, sizeof(messageLength));
        if(n <= 0)
        {
            perror("ERROR reading from socket");

            std::cerr << curUserName << "已下线；" << "Client：" << ip << std::endl;

            //将当前线程用户从已登录用户表中移除
            DBManager::instance()->removeLoggedInUser(curUserName);

            // 从 m_clientSockets 中移除断开连接的套接字
            auto iter = std::find(m_clientSockets.begin(), m_clientSockets.end(), clientSockfd);
            if (iter != m_clientSockets.end())
            {
                m_clientSockets.erase(iter);
            }

            //发送当前在线用户给所有人
            sendOnlineUser();

            break;
        }

        //将网络字节顺序转换为主机字节顺序
        uint32_t dataLength = ntohl(messageLength);

        // 根据长度读取实际消息
        std::string buffer(dataLength, '\0');
        n = read(clientSockfd, &buffer[0], dataLength);
        if(n <= 0)
        {
            perror("ERROR reading from socket");
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

            //只在第一次循环时获取当前线程的用户名
            if(circleCount == 1)
            {
                curUserName = userName;
            }

            switch(messageType)
            {
                case Login:
                {
                    //处理登录
                    handleLogin(clientSockfd, userName, passWord);
                    break;
                }
                case SignUp:
                {
                    //处理注册
                    handleSignUp(clientSockfd, userName, passWord);
                    break;
                }
                case Normal:
                {
                    //处理正常聊天信息
                    handleChatMessages(buffer);
                    break;
                }
                case FileStart:
                {
                    //处理文件开始
                    handleFileStart(buffer);
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

        circleCount++;
    }
    close(clientSockfd);
}

//发送当前在线用户给所有人
void Server::sendOnlineUser()
{
    std::vector<std::string> onlineUsers = DBManager::instance()->getLoggedInUserAllRecords();
    Json::Value root;
    Json::Value usernames(Json::arrayValue);
    Json::Value messages(Json::objectValue);

    for (const auto& user : onlineUsers)
    {
        usernames.append(user);
    }

    messages["usernames"] = usernames;
    root["type"] = OnlineUsers;
    root["messages"] = messages;

    std::string jsonString = Json::writeString(Json::StreamWriterBuilder(), root);

    for (int sockfd : m_clientSockets)
    {
        sendWithLengthPrefix(sockfd, jsonString);
    }
}

//发送登录结果
void Server::sendLoginResult(int sockfd, LoginResultType type, const std::string &userName)
{
    Json::Value root;
    root["type"] = LoginResult;

    Json::Value messages;
    messages["code"] = type;
    messages["username"] = userName;

    root["messages"] = messages;

    //将JSON对象转换为字符串
    Json::StreamWriterBuilder builder;
    std::string jsonString = Json::writeString(builder, root);

    sendWithLengthPrefix(sockfd, jsonString);
}

//发送注册结果
void Server::sandSingUpResult(int sockfd, LoginResultType type)
{
    Json::Value root;
    root["type"] = SignupResult;

    Json::Value messages;
    messages["code"] = type;

    root["messages"] = messages;

    //将JSON对象转换为字符串
    Json::StreamWriterBuilder builder;
    std::string jsonString = Json::writeString(builder, root);

    sendWithLengthPrefix(sockfd, jsonString);
}

//服务端发送消息
void Server::sendWithLengthPrefix(int sockfd, const std::string &message)
{
    uint32_t length = htonl(static_cast<uint32_t>(message.size()));

    std::cout << "发送：" << message << std::endl;

    //发送消息长度
    ssize_t bytesSent = write(sockfd, &length, sizeof(length));
    if(bytesSent < 0)
    {
        //发送失败，处理错误
        perror("Error sending message length");
        return;
    }
    else if(bytesSent < sizeof(length))
    {
        //未完全发送
        std::cerr << "Incomplete write for message length" << std::endl;
        return;
    }

    //发送消息本身
    bytesSent = write(sockfd, message.c_str(), message.size());
    if(bytesSent < 0)
    {
        //发送失败，处理错误
        perror("Error sending message");
        return;
    }
    else if(static_cast<size_t>(bytesSent) < message.size())
    {
        //未完全发送
        std::cerr << "Incomplete write for message" << std::endl;
        return;
    }
}

//处理正常聊天信息
void Server::handleChatMessages(const std::string &buffer)
{
    Json::Value root;
    Json::Reader reader;

    if(!reader.parse(buffer, root))
    {
        std::cerr << "Failed to parse buffer: " << reader.getFormattedErrorMessages();
        return;
    }

    std::string currentTimestamp = getCurrentTimestamp();
    root["messages"]["date"] = currentTimestamp;

    //组合为新json
    std::string newJson = root.toStyledString();

    if(reader.parse(newJson, root))
    {
        //解析type
        int type = root["type"].asInt();

        //解析message
        Json::Value messages = root["messages"];
        Json::StreamWriterBuilder builder;
        std::string message = Json::writeString(builder, messages);

        //解析用户名
        std::string userName = root["messages"]["username"].asString();

        //存入聊天记录表
        if(!DBManager::instance()->addSingleChatHistory(userName, message, type, currentTimestamp))
        {
            return;
        }

        //转发给所有客户端
        for(int sockfd : m_clientSockets)
        {
            sendWithLengthPrefix(sockfd, newJson);
        }
    }
    else
    {
        std::cerr << "Failed to parse JSON: " << reader.getFormattedErrorMessages();
    }
}

//获取当前时间
std::string Server::getCurrentTimestamp()
{
    //获取当前时间点
    auto now = std::chrono::system_clock::now();

    //转换为时间_t对象
    auto now_c = std::chrono::system_clock::to_time_t(now);

    //将时间转换为tm结构
    std::tm now_tm = *std::localtime(&now_c);

    //使用stringstream格式化时间
    std::stringstream ss;
    ss << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S");

    return ss.str();
}

//获取最新的20条聊天记录
void Server::sendLastedChatMessages(int sockfd)
{
    std::vector<std::string> messagesVector = DBManager::instance()->getRecordsFormChatHistory(20);

    if(messagesVector.empty())
    {
        return;
    }

    for(const auto& chatRecord : messagesVector)
    {
        sendWithLengthPrefix(sockfd, chatRecord);
    }
}

//处理文件开始
void Server::handleFileStart(const std::string &buffer)
{
    Json::Value root;
    Json::Reader reader;

    if(!reader.parse(buffer, root))
    {
        std::cerr << "Failed to parse file buffer: " << reader.getFormattedErrorMessages();
        return;
    }

    //时间戳
    std::string currentTimestamp = getCurrentTimestamp();
    //文件名和文件大小
    std::string fileName = root["messages"]["fileName"].asString();
    long fileSize = root["messages"]["fileSize"].asInt64();

    std::cout << currentTimestamp << fileName << fileSize;

}


