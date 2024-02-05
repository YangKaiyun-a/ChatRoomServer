//
// Created by yky on 24-1-30.
//

#ifndef CHATROOMSERVER_SERVER_H
#define CHATROOMSERVER_SERVER_H

#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <vector>
#include "global.h"


class Server {

public:
    explicit Server(int port);
    ~Server();
    void start();
    //处理登录逻辑
    void handleLogin(int sockfd, const std::string &userName, const std::string &password);
    //发送登录结果
    static void sendLoginResult(int sockfd, LoginResultType type, const std::string &userName);
    //发送在线人数
    void sendOnlineUser();
    //处理注册逻辑
    static void handleSignUp(int sockfd, const std::string &userName, const std::string &password);
    //发送登录结果
    static void sandSingUpResult(int sockfd, LoginResultType type);
    //与每一个用户进行通信
    void handleClient(int clientSockfd, char *ip);
    //发送消息
    static void sendWithLengthPrefix(int sockfd, const std::string &message);
    //处理正常聊天信息
    void handleNormalMessages(const std::string &message);
    //获取当前时间
    static std::string getCurrentTimestamp();
    //发送最新20条聊天记录给新登录用户
    static void sendLastedChatMessages(int sockfd);

private:
    int m_sockfd, m_newsockfd{}, m_port;
    socklen_t clilen{};
    struct sockaddr_in serv_addr{}, cli_addr{};
    std::vector<int> m_clientSockets;

    void error(const char *msg) {
        perror(msg);
        exit(1);
    }


};


#endif //CHATROOMSERVER_SERVER_H
