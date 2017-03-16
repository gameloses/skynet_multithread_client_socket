//
//  SocketCommunication.hpp
//  base
//
//  Created by dmx on 2017/3/14.
//  Copyright © 2017年 everding. All rights reserved.
//

#ifndef SocketCommunication_hpp
#define SocketCommunication_hpp

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <map>
#include <vector>
#include <string>

#include <pthread.h>

#include "SKSocket.hpp"
using namespace std;

class SocketCommunication {
public:
    SocketCommunication();
    virtual ~SocketCommunication();
    static SocketCommunication * getInstance();
    bool connect(string socketName, string host, int port);
    char *read(string &socketName);
    void write(string &socketName, const char *content);
    
private:
    
    map<string, SKSocket *> sockets;
    map<string, vector<string> *> responseQueues;
    map<string, vector<string> *> requestQueues;

    pthread_t readThread;
    pthread_t writeThread;
    pthread_mutex_t readMutex;
    pthread_mutex_t writeMutex;
    
    bool startInternalThreads();
    static void* readFunc(void *data);
    void internalReadFunc();
    static void* writeFunc(void *data);
    void internalWriteFunc();
    
    inline void lockRead() {
        pthread_mutex_lock(&readMutex);
    }
    
    inline void unlockRead() {
        pthread_mutex_unlock(&readMutex);
    }
    
    inline void lockWrite() {
        pthread_mutex_lock(&writeMutex);
    }
    
    inline void unlockWrite() {
        pthread_mutex_unlock(&writeMutex);
    }
    
    void popResponse(string, char **);
    void popRequest(string, char **);
    void pushRequest(string, string &);
    void pushResponse(string, string &);
    
    
    
};
#endif /* SocketCommunication_hpp */
