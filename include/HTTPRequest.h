//
//  HTTPRequest.h
//  test
//
//  Created by Elviss Strazdins on 15/05/2017.
//  Copyright © 2017 Elviss Strazdins. All rights reserved.
//

#pragma once

#include <iostream>

#ifdef _MSC_VER
#define NOMINMAX
#include <winsock2.h>
typedef SOCKET socket_t;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
typedef int socket_t;
#define INVALID_SOCKET -1
#endif

inline int getLastError()
{
#ifdef _MSC_VER
    return WSAGetLastError();
#else
    return errno;
#endif
}

#ifdef _MSC_VER
inline bool initWSA()
{
    WORD sockVersion = MAKEWORD(2, 2);
    WSADATA wsaData;
    int error = WSAStartup(sockVersion, &wsaData);
    if (error != 0)
    {
        Log(Log::Level::ERR) << "WSAStartup failed, error: " << error << std::endl;
        return false;
    }

    if (wsaData.wVersion != sockVersion)
    {
        Log(Log::Level::ERR) << "Incorrect Winsock version" << std::endl;
        WSACleanup();
        return false;
    }

    return true;
}
#endif

class HTTPRequest
{
public:
    HTTPRequest(std::string url)
    {
        auto protocolEndPosition = url.find("://");

        if (protocolEndPosition != std::string::npos)
        {
            protocol = url.substr(0, protocolEndPosition);

            auto pathPosition = url.find('/', protocolEndPosition + 3);

            if (pathPosition == std::string::npos)
            {
                domain = url.substr(protocolEndPosition + 3);
            }
            else
            {
                domain = url.substr(protocolEndPosition + 3, pathPosition - protocolEndPosition - 3);
                path = url.substr(pathPosition);
            }

            auto portPosition = domain.find(':');

            if (portPosition != std::string::npos)
            {
                port = static_cast<uint16_t>(std::stoi(domain.substr(portPosition + 1)));
                domain.resize(portPosition);
            }
        }
    }

    ~HTTPRequest()
    {
#ifdef _MSC_VER
        if (socketFd != INVALID_SOCKET) closesocket(socketFd);
#else
        if (socketFd != INVALID_SOCKET) close(socketFd);
#endif
    }

    HTTPRequest(const HTTPRequest& httpRequest) = delete;
    HTTPRequest(HTTPRequest&& httpRequest) = delete;
    HTTPRequest& operator=(const HTTPRequest& httpRequest) = delete;
    HTTPRequest& operator=(HTTPRequest&& httpRequest) = delete;

    bool send()
    {
        socket_t socketFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

#ifdef _MSC_VER
        if (socketFd == INVALID_SOCKET && WSAGetLastError() == WSANOTINITIALISED)
        {
            if (!initWSA()) return false;

            socketFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        }
#endif

        if (socketFd == INVALID_SOCKET)
        {
            int error = getLastError();
            std::cerr << "Failed to create socket, error: " << error << std::endl;
            return false;
        }



        return true;
    }

private:
    std::string protocol;
    std::string domain;
    uint16_t port = 80;
    std::string path;
    socket_t socketFd = INVALID_SOCKET;
};
