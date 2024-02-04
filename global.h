//
// Created by yky on 24-1-30.
//

#ifndef CHATROOMSERVER_GLOBAL_H
#define CHATROOMSERVER_GLOBAL_H

enum MessageType
{
    Login,          //登录
    SignUp,         //注册
    Normal,         //正常聊天类型
    LoginResult,    //登录结果
    SignupResult,   //注册结果
    OnlineUsers     //在线人数
};

enum LoginResultType
{
    Login_Successed,    //登录成功
    SignUp_Successed,   //注册成功
    Error_Password,     //密码错误
    Unknow_UserName,    //不存在该用户
    Exist_UserName,     //已存在该用户
    AlreadyLoggedIn,    //该用户已登录
    Erro_Sql            //网络错误
};

#endif //CHATROOMSERVER_GLOBAL_H
