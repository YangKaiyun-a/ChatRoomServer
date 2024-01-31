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
    static void handleLogin(int sockfd, LoginResultType type);
    static void handleSignUp(int sockfd, LoginResultType type);
    void handleClient(int clientSockfd, char *ip);

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
