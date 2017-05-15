//
//  HTTPRequest
//

#pragma once

#include <iostream>
#include <map>
#include <vector>

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

namespace http
{
    struct Response
    {
        bool succeeded = false;
        std::vector<std::string> headers;
        std::vector<uint8_t> body;
    };

    class Request
    {
    public:
        Request(const std::string& url)
        {
            size_t protocolEndPosition = url.find("://");

            if (protocolEndPosition != std::string::npos)
            {
                protocol = url.substr(0, protocolEndPosition);
                std::transform(protocol.begin(), protocol.end(), protocol.begin(), ::tolower);

                size_t pathPosition = url.find('/', protocolEndPosition + 3);

                if (pathPosition == std::string::npos)
                {
                    domain = url.substr(protocolEndPosition + 3);
                }
                else
                {
                    domain = url.substr(protocolEndPosition + 3, pathPosition - protocolEndPosition - 3);
                    path = url.substr(pathPosition);
                }

                size_t portPosition = domain.find(':');

                if (portPosition != std::string::npos)
                {
                    port = domain.substr(portPosition + 1);
                    domain.resize(portPosition);
                }
            }
        }

        ~Request()
        {
#ifdef _MSC_VER
            if (socketFd != INVALID_SOCKET) closesocket(socketFd);
#else
            if (socketFd != INVALID_SOCKET) close(socketFd);
#endif
        }

        Request(const Request& request) = delete;
        Request(Request&& request) = delete;
        Request& operator=(const Request& request) = delete;
        Request& operator=(Request&& request) = delete;

        Response send(const std::string& method,
                      const std::string& body,
                      const std::vector<std::string>& headers)
        {
            Response response;

            if (protocol != "http")
            {
                std::cerr << "Only HTTP protocol is supported" << std::endl;
                return response;
            }

            if (socketFd != INVALID_SOCKET)
            {
#ifdef _MSC_VER
                int result = closesocket(socketFd);
#else
                int result = ::close(socketFd);
#endif
                socketFd = INVALID_SOCKET;

                if (result < 0)
                {
                    int error = getLastError();
                    std::cerr << "Failed to close socket, error: " << error << std::endl;
                    return response;
                }
            }

            socketFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

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
                return response;
            }

            addrinfo* info;
            if (getaddrinfo(domain.c_str(), port.empty() ? nullptr : port.c_str(), nullptr, &info) != 0)
            {
                int error = getLastError();
                std::cerr << "Failed to get address info of " << domain << ", error: " << error << std::endl;
                return response;
            }

            sockaddr addr = *info->ai_addr;

            freeaddrinfo(info);

            if (::connect(socketFd, &addr, sizeof(addr)) < 0)
            {
                int error = getLastError();

                std::cerr << "Failed to connect to " << domain << ":" << port << ", error: " << error << std::endl;
                return response;
            }
            else
            {
                std::cerr << "Connected to to " << domain << ":" << port << std::endl;
            }

            std::string data = method + " " + path + " HTTP/1.1\r\n";

            for (const std::string& header : headers)
            {
                data += header + "\r\n";
            }

            data += "Content-Length:" + std::to_string(body.size()) + "\r\n";

            data += "\r\n";
            data += body + "\r\n";

#if defined(__APPLE__)
            int flags = 0;
#elif defined(_MSC_VER)
            int flags = 0;
#else
            int flags = MSG_NOSIGNAL;
#endif

#ifdef _MSC_VER
            int remaining = static_cast<int>(data.size());
            int sent = 0;
#else
            ssize_t remaining = static_cast<ssize_t>(data.size());
            ssize_t sent = 0;
#endif

            do
            {
#ifdef _MSC_VER
                int size = ::send(socketFd, data.data() + sent, remaining, flags);
#else
                ssize_t size = ::send(socketFd, data.data() + sent, remaining, flags);
#endif

                if (size < 0)
                {
                    int error = getLastError();
                    std::cerr << "Failed to send data to " << domain << ":" << port << ", error: " << error << std::endl;
                    return response;
                }

                remaining -= size;
                sent += size;
            }
            while (remaining > 0);

            uint8_t TEMP_BUFFER[65536];

            do
            {
#ifdef _MSC_VER
                int size = recv(socketFd, reinterpret_cast<char*>(TEMP_BUFFER), sizeof(TEMP_BUFFER), flags);
#else
                ssize_t size = recv(socketFd, reinterpret_cast<char*>(TEMP_BUFFER), sizeof(TEMP_BUFFER), flags);
#endif

                if (size < 0)
                {
                    int error = getLastError();
                    std::cerr << "Failed to read data from " << domain << ":" << port << ", error: " << error << std::endl;
                    return response;
                }
                else if (size == 0)
                {
                    // disconnected
                    break;
                }

                response.body.insert(response.body.end(), std::begin(TEMP_BUFFER), std::end(TEMP_BUFFER) + size);
            }
            while (1);

            response.succeeded = true;

            return response;
        }

    private:
        std::string protocol;
        std::string domain;
        std::string port = "80";
        std::string path;
        socket_t socketFd = INVALID_SOCKET;
    };
}
