//
//  SKSocket.cpp
//  base
//
//  Created by dmx on 2017/3/14.
//  Copyright © 2017年 丁明信. All rights reserved.
//

#include "SKSocket.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>


#define INVALID_SOCKET -1
#define SOCKET_ERROR -1

SKSocket::SKSocket(string n, string h, int p):name(n),host(h),port(p)
{
}


SKSocket::~SKSocket()
{

}

// 解析host，并connect
bool SKSocket::connectOnce()
{
    struct addrinfo *result = NULL, *ptr = NULL, hints;
    char portStr[25];
    sprintf(portStr, "%d", this->port);
    
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    int resolveResult = getaddrinfo(this->host.c_str(), portStr, &hints, &result);
    if(resolveResult != 0) {
        return false;
    }
    
    int sockId = INVALID_SOCKET;
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        struct sockaddr_in *addr = (struct sockaddr_in *) ptr->ai_addr;
        //turn iner addr num to addr ip addr
//        char *addr_str = inet_ntoa(addr->sin_addr);
//        printf("dmx===ai_addr %d addr_str:%s\n", addr->sin_addr.s_addr, addr_str);
        
        // create socket
        sockId = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if(sockId != INVALID_SOCKET) {
            // connect to socket
            int connectResult = connect(sockId, ptr->ai_addr, (int)ptr->ai_addrlen);
            if (connectResult == SOCKET_ERROR) {
                close(sockId);
                sockId = INVALID_SOCKET;
            }
            else {
                break;
            }
        }
    }
    bool ret = false;
    if (sockId != INVALID_SOCKET) {
        this->socketId = sockId;
        this->setoptNoSignal();
        ret = true;
    }
    freeaddrinfo(result);
    
    return true;
}

bool SKSocket::setoptNoSignal()
{
    //服务器异常断开时， 客户端如果还发请求，会raise sigal 给系统，系统对signal的处理往往是终止线程，
    //设置SO_NOSIGPIPE 这个选项就是为了避免这个事情的
    int nosignal = 1;
    int ret = setsockopt(this->socketId, SOL_SOCKET, SO_NOSIGPIPE, (void *)&nosignal, sizeof(nosignal));
    return ret != -1;
}

void SKSocket::write(char * content)
{

    this->doSend((const char *)content);
}

char * SKSocket::read()
{
    char *msg = this->doRecv();
    return msg;
}

void SKSocket::doSend(const char *content)
{
    int len = (int)strlen(content);
    int real_len = len+2;
    char msg[real_len];
    msg[0] = (len >> 8)& 0xff;
    msg[1] = len & 0xff;
    memcpy(msg+2, content, len);
    int count = 0;
    int bytes = 0;
    
    while (count < real_len) {
        bytes = send(this->socketId, msg+count, real_len-count, 0);
        if (bytes == -1 || bytes == 0) {
            //TODO socket error
            //此处可以判断socket 已经断开了
            printf("%d bytes -------\n", bytes);
            break;
        }
        count += bytes;
    }
}

int SKSocket::doSelect()
{
    if(this->socketId == INVALID_SOCKET) {
        return -1;
    }
    
    FD_ZERO(&this->fdr);
    FD_SET(this->socketId, &this->fdr);
    
    int result = select(this->socketId + 1, &this->fdr, NULL, NULL, NULL);
    if(result == -1) {
        return -1;
    } else {
        if(FD_ISSET(this->socketId, &this->fdr)) {
            return -2;
        } else {
            return -3;
        }
    }
}

char * SKSocket::doRecv()
{
    while(true) {
        int selectResult = this->doSelect();
        if (selectResult == -2) {
            char lenBuf[2];
            int size = (int)recv(this->socketId, lenBuf, 2, 0);
            if (size == 2 ) {
                int msgBodyLen = (int)(lenBuf[0]<<8 | lenBuf[1]);
                if (msgBodyLen > 0) {
                    char *msgBodyBuf = new char[msgBodyLen];
                    size = (int)recv(this->socketId, msgBodyBuf, msgBodyLen, 0);
                    if (size > 0) {
                        auto messageResult = string(msgBodyBuf, msgBodyLen);
                        delete msgBodyBuf;
                        return (char *)messageResult.c_str();
                    }
                }
            }
            else {
                //TODO return error
                return NULL;
            }
        }
    }
    return NULL;
}
