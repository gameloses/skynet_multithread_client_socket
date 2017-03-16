//
//  SocketCommunication.cpp
//  base
//
//  Created by dmx on 2017/3/14.
//  Copyright © 2017年 丁明信. All rights reserved.
//

#include "SocketCommunication.hpp"
#include "SKSocket.hpp"
static SocketCommunication *m_instance;

SocketCommunication::SocketCommunication()
{
    this->startInternalThreads();
}

SocketCommunication::~SocketCommunication()
{
    //TODO
    //stop read and write thread
    // clear read and write queue
}

SocketCommunication * SocketCommunication::getInstance()
{
    if (m_instance == NULL) {
        m_instance = new SocketCommunication();
    }
    return m_instance;
}


bool SocketCommunication::connect(string socketName, string host, int port)
{
    SKSocket *socket = new SKSocket(socketName, host, port);
    bool result = false;
    result = socket->connectOnce();
    if (! result) {
        return result;
    }
    this->sockets[socketName] = socket;
    this->requestQueues[socketName] = new vector<string>();
    this->responseQueues[socketName] = new vector<string>();
    return true;
}

char * SocketCommunication::read(string &socketName)
{
    vector<string> *rq = this->responseQueues[socketName];
    if (rq != NULL) {
        vector<string> &responseQueue = *rq;
        if (responseQueue.size() > 0) {
            char *res = (char *)responseQueue.front().c_str();
            responseQueue.erase(responseQueue.begin());
            return res;
        }
    }
    return NULL;
}

void SocketCommunication::write(const char *content, string &socketName)
{
    string data = content;
    this->pushRequest(socketName, data);
}

bool SocketCommunication::startInternalThreads()
{
    bool readInit = false;
    bool writeInit = false;
    if (pthread_create(&this->readThread, NULL, readFunc, this) == 0) {
        pthread_mutex_init(&this->readMutex, NULL);
        readInit = true;
    }
    if (pthread_create(&this->writeThread, NULL, writeFunc, this) == 0) {
        pthread_mutex_init(&this->writeMutex, NULL);
        writeInit = true;
    }
    return readInit && writeInit;
}

void * SocketCommunication::readFunc(void *This)
{
    ((SocketCommunication *)This)->internalReadFunc();
    return NULL;
}

void SocketCommunication::internalReadFunc()
{
    while (true) {
        usleep(1000);
        
        for (auto const &info : this->sockets) {
            string socketName = info.first;
            SKSocket *socket = info.second;
            char *msg = socket->read();
            if (msg != NULL) {
                string content(msg);
                this->pushResponse(socketName, content); 
            }
        }
    }
}

void * SocketCommunication::writeFunc(void *This)
{
    ((SocketCommunication *)This)->internalWriteFunc();
    return NULL;
}

void SocketCommunication::internalWriteFunc()
{
    while (true) {
        usleep(1000);
        for (auto const &info : this->sockets) {
            string socketName = info.first;
            char *request = NULL;
            this->popRequest(socketName, &request);
            if (request) {
                SKSocket *socket = info.second;
                socket->write(request);
            }
        }
    }
}

void SocketCommunication::pushRequest(string socketName, string &content)
{
    lockWrite();
    vector<string> *rq = this->requestQueues[socketName];
    vector<string> &requestQueue = *rq;
    if (rq != NULL) {
        string req(content.c_str());
        requestQueue.push_back(req);
    }
    unlockWrite();
}

void SocketCommunication::pushResponse(string socketName, string &content)
{
    lockRead();
    vector<string> *rq = this->responseQueues[socketName];
    
    if (rq != NULL) {
        vector<string> &responseQueue = *rq;
        string res = content;
        responseQueue.push_back(res);

    }
    unlockRead();
}

void SocketCommunication::popRequest(string socketName, char **request)
{
    lockWrite();
    vector<string> *rq = this->requestQueues[socketName];

    if(rq != NULL) {
        vector<string> &requestQueue = *rq;
        if (requestQueue.size() > 0) {
            string data = requestQueue.front();
            *request = (char *)data.c_str();
            requestQueue.erase(requestQueue.begin());
        }

    }
    unlockWrite();
}

void SocketCommunication::popResponse(string socketName, char **response)
{
    lockRead();
    vector<string> *rq = this->responseQueues[socketName];

    if (rq != NULL) {
        vector<string> &responseQueue = *rq;
        if (responseQueue.size() > 0) {
            string data = responseQueue.front();
            *response = (char *)data.c_str();
            responseQueue.erase(responseQueue.begin());
        }

    }
    unlockRead();
}
