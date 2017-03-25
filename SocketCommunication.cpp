//
//  SocketCommunication.cpp
//  base
//
//  Created by dmx on 2017/3/14.
//  Copyright © 2017年 everding. All rights reserved.
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

string SocketCommunication::read(string &socketName)
{
    return this->popResponse(socketName);
}

void SocketCommunication::write(string &socketName, const char *content)
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
            string msg = socket->read();
            if (msg.length() > 0) {
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
            string request = this->popRequest(socketName);
            if (request.length() > 0) {
                SKSocket *socket = info.second;
                socket->write((char *)request.c_str());
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
        string *res = new string(content);
        responseQueue.push_back(*res);

    }
    unlockRead();
}

string SocketCommunication::popRequest(string socketName)
{
    lockWrite();
    vector<string> *rq = this->requestQueues[socketName];

    if(rq != NULL) {
        vector<string> &requestQueue = *rq;
        if (requestQueue.size() > 0) {
            string data = requestQueue.front();
            auto request = string(data);
            requestQueue.erase(requestQueue.begin());
            unlockWrite();
            return request;
        }

    }
    unlockWrite();
    return "";
}

string SocketCommunication::popResponse(string &socketName)
{
    lockRead();
    vector<string> *rq = this->responseQueues[socketName];

    if (rq != NULL) {
        vector<string> &responseQueue = *rq;
        if (responseQueue.size() > 0) {
            string data = responseQueue.front();
            auto response = string(data);
            responseQueue.erase(responseQueue.begin());
            unlockRead();
            return response;
        }

    }
    unlockRead();
    return "";
}
