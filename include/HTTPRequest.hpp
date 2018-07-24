//
//  HTTPRequest
//

#pragma once

#include <algorithm>
#include <functional>
#include <stdexcept>
#include <map>
#include <string>
#include <vector>
#include <cstdint>
#include <cctype>

#ifdef _WIN32
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
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

namespace http
{
    inline int getLastError()
    {
#ifdef _WIN32
        return WSAGetLastError();
#else
        return errno;
#endif
    }

#ifdef _WIN32
    inline void initWSA()
    {
        WORD sockVersion = MAKEWORD(2, 2);
        WSADATA wsaData;
        int error = WSAStartup(sockVersion, &wsaData);
        if (error != 0)
            throw std::runtime_error("WSAStartup failed, error: " + std::to_string(error));

        if (wsaData.wVersion != sockVersion)
            throw std::runtime_error("Incorrect Winsock version");
    }
#endif
    
    inline std::string urlEncode(const std::string& str)
    {
        static const std::map<char, std::string> entities = {
            {' ', "%20"},
            {'!', "%21"},
            {'"', "%22"},
            {'*', "%2A"},
            {'\'', "%27"},
            {'(', "%28"},
            {')', "%29"},
            {';', "%3B"},
            {':', "%3A"},
            {'@', "%40"},
            {'&', "%26"},
            {'=', "%3D"},
            {'+', "%2B"},
            {'$', "%24"},
            {',', "%2C"},
            {'/', "%2F"},
            {'?', "%3F"},
            {'%', "%25"},
            {'#', "%23"},
            {'<', "%3C"},
            {'>', "%3E"},
            {'[', "%5B"},
            {'\\', "%5C"},
            {']', "%5D"},
            {'^', "%5E"},
            {'`', "%60"},
            {'{', "%7B"},
            {'|', "%7C"},
            {'}', "%7D"},
            {'~', "%7E"}
        };

        static const char hexChars[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

        std::string result;

        for (auto i = str.begin(); i != str.end(); ++i)
        {
            uint32_t cp = *i & 0xff;

            if (cp <= 0x7f) // length = 1
            {
                auto entity = entities.find(*i);
                if (entity == entities.end())
                    result += static_cast<char>(cp);
                else
                    result += entity->second;
            }
            else if ((cp >> 5) == 0x6) // length = 2
            {
                result += std::string("%") + hexChars[(*i & 0xf0) >> 4] + hexChars[*i & 0x0f];
                if (++i == str.end()) break;
                result += std::string("%") + hexChars[(*i & 0xf0) >> 4] + hexChars[*i & 0x0f];
            }
            else if ((cp >> 4) == 0xe) // length = 3
            {
                result += std::string("%") + hexChars[(*i & 0xf0) >> 4] + hexChars[*i & 0x0f];
                if (++i == str.end()) break;
                result += std::string("%") + hexChars[(*i & 0xf0) >> 4] + hexChars[*i & 0x0f];
                if (++i == str.end()) break;
                result += std::string("%") + hexChars[(*i & 0xf0) >> 4] + hexChars[*i & 0x0f];
            }
            else if ((cp >> 3) == 0x1e) // length = 4
            {
                result += std::string("%") + hexChars[(*i & 0xf0) >> 4] + hexChars[*i & 0x0f];
                if (++i == str.end()) break;
                result += std::string("%") + hexChars[(*i & 0xf0) >> 4] + hexChars[*i & 0x0f];
                if (++i == str.end()) break;
                result += std::string("%") + hexChars[(*i & 0xf0) >> 4] + hexChars[*i & 0x0f];
                if (++i == str.end()) break;
                result += std::string("%") + hexChars[(*i & 0xf0) >> 4] + hexChars[*i & 0x0f];
            }
        }
        
        return result;
    }

    struct Response
    {
        int code = 0;
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

                std::string::size_type pathPosition = url.find('/', protocolEndPosition + 3);

                if (pathPosition == std::string::npos)
                    domain = url.substr(protocolEndPosition + 3);
                else
                {
                    domain = url.substr(protocolEndPosition + 3, pathPosition - protocolEndPosition - 3);
                    path = url.substr(pathPosition);
                }

                std::string::size_type portPosition = domain.find(':');

                if (portPosition != std::string::npos)
                {
                    port = domain.substr(portPosition + 1);
                    domain.resize(portPosition);
                }
            }
        }

        ~Request()
        {
            if (socketFd != INVALID_SOCKET)
            {
#ifdef _WIN32
                closesocket(socketFd);
#else
                close(socketFd);
#endif
            }
        }

        Request(const Request& request) = delete;
        Request(Request&& request) = delete;
        Request& operator=(const Request& request) = delete;
        Request& operator=(Request&& request) = delete;

        Response send(const std::string& method,
                      const std::map<std::string, std::string>& parameters,
                      const std::vector<std::string>& headers = {})
        {
            std::string body;
            bool first = true;

            for (const auto& parameter : parameters)
            {
                if (!first) body += "&";
                first = false;

                body += urlEncode(parameter.first) + "=" + urlEncode(parameter.second);
            }

            return send(method, body, headers);
        }

        Response send(const std::string& method,
                      const std::string& body = "",
                      const std::vector<std::string>& headers = {})
        {
            Response response;

            if (protocol != "http")
                throw std::runtime_error("Only HTTP protocol is supported");

            if (socketFd != INVALID_SOCKET)
            {
#ifdef _WIN32
                int result = closesocket(socketFd);
#else
                int result = ::close(socketFd);
#endif
                socketFd = INVALID_SOCKET;

                if (result < 0)
                {
                    int error = getLastError();
                    throw std::runtime_error("Failed to close socket, error: " + std::to_string(error));
                }
            }

            socketFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

#ifdef _WIN32
            if (socketFd == INVALID_SOCKET && WSAGetLastError() == WSANOTINITIALISED)
            {
                initWSA();
                socketFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            }
#endif

            if (socketFd == INVALID_SOCKET)
            {
                int error = getLastError();
                throw std::runtime_error("Failed to create socket, error: " + std::to_string(error));
            }

            addrinfo* info;
            if (getaddrinfo(domain.c_str(), port.empty() ? nullptr : port.c_str(), nullptr, &info) != 0)
            {
                int error = getLastError();
                throw std::runtime_error("Failed to get address info of " + domain + ", error: " + std::to_string(error));
            }

            sockaddr addr = *info->ai_addr;

            freeaddrinfo(info);

            if (::connect(socketFd, &addr, sizeof(addr)) < 0)
            {
                int error = getLastError();
                throw std::runtime_error("Failed to connect to " + domain + ":" + port + ", error: " + std::to_string(error));
            }

            std::string requestData = method + " " + path + " HTTP/1.1\r\n";

            for (const std::string& header : headers)
                requestData += header + "\r\n";

            requestData += "Host: " + domain + "\r\n";
            requestData += "Content-Length: " + std::to_string(body.size()) + "\r\n";

            requestData += "\r\n";
            requestData += body;

#if defined(__APPLE__)
            int flags = 0;
#elif defined(_WIN32)
            int flags = 0;
#else
            int flags = MSG_NOSIGNAL;
#endif

#ifdef _WIN32
            int remaining = static_cast<int>(requestData.size());
            int sent = 0;
            int size;
#else
            ssize_t remaining = static_cast<ssize_t>(requestData.size());
            ssize_t sent = 0;
            ssize_t size;
#endif

            do
            {
                size = ::send(socketFd, requestData.data() + sent, static_cast<size_t>(remaining), flags);

                if (size < 0)
                {
                    int error = getLastError();
                    throw std::runtime_error("Failed to send data to " + domain + ":" + port + ", error: " + std::to_string(error));
                }

                remaining -= size;
                sent += size;
            }
            while (remaining > 0);

            uint8_t TEMP_BUFFER[65536];
            const std::vector<uint8_t> clrf = {'\r', '\n'};
            std::vector<uint8_t> responseData;
            bool firstLine = true;
            bool parsedHeaders = false;
            int contentSize = -1;
            bool chunkedResponse = false;
            size_t expectedChunkSize = 0;
            bool removeCLRFAfterChunk = false;

            do
            {
                size = recv(socketFd, reinterpret_cast<char*>(TEMP_BUFFER), sizeof(TEMP_BUFFER), flags);

                if (size < 0)
                {
                    int error = getLastError();
                    throw std::runtime_error("Failed to read data from " + domain + ":" + port + ", error: " + std::to_string(error));
                }
                else if (size == 0)
                {
                    // disconnected
                    break;
                }

                responseData.insert(responseData.end(), std::begin(TEMP_BUFFER), std::begin(TEMP_BUFFER) + size);

                if (!parsedHeaders)
                {
                    for (;;)
                    {
                        std::vector<uint8_t>::iterator i = std::search(responseData.begin(), responseData.end(), clrf.begin(), clrf.end());

                        // didn't find a newline
                        if (i == responseData.end()) break;

                        std::string line(responseData.begin(), i);
                        responseData.erase(responseData.begin(), i + 2);

                        // empty line indicates the end of the header section
                        if (line.empty())
                        {
                            parsedHeaders = true;
                            break;
                        }
                        else if (firstLine) // first line
                        {
                            firstLine = false;

                            std::string::size_type pos, lastPos = 0, length = line.length();
                            std::vector<std::string> parts;

                            // tokenize first line
                            while (lastPos < length + 1)
                            {
                                pos = line.find(' ', lastPos);
                                if (pos == std::string::npos) pos = length;

                                if (pos != lastPos)
                                    parts.push_back(std::string(line.data() + lastPos,
                                                                static_cast<std::vector<std::string>::size_type>(pos) - lastPos));
                                
                                lastPos = pos + 1;
                            }

                            if (parts.size() >= 2)
                                response.code = std::stoi(parts[1]);
                        }
                        else // headers
                        {
                            response.headers.push_back(line);

                            std::string::size_type pos = line.find(':');

                            if (pos != std::string::npos)
                            {
                                std::string headerName = line.substr(0, pos);
                                std::string headerValue = line.substr(pos + 1);

                                // ltrim
                                headerValue.erase(headerValue.begin(),
                                                  std::find_if(headerValue.begin(), headerValue.end(),
                                                               std::not1(std::ptr_fun<int, int>(std::isspace))));

                                // rtrim
                                headerValue.erase(std::find_if(headerValue.rbegin(), headerValue.rend(),
                                                               std::not1(std::ptr_fun<int, int>(std::isspace))).base(),
                                                  headerValue.end());

                                if (headerName == "Content-Length")
                                    contentSize = std::stoi(headerValue);
                                else if (headerName == "Transfer-Encoding" && headerValue == "chunked")
                                    chunkedResponse = true;
                            }
                        }
                    }
                }

                if (parsedHeaders)
                {
                    if (chunkedResponse)
                    {
                        bool dataReceived = false;
                        for (;;)
                        {
                            if (expectedChunkSize > 0)
                            {
                                auto toWrite = std::min(expectedChunkSize, responseData.size());
                                response.body.insert(response.body.end(), responseData.begin(), responseData.begin() + static_cast<ssize_t>(toWrite));
                                responseData.erase(responseData.begin(), responseData.begin() + static_cast<ssize_t>(toWrite));
                                expectedChunkSize -= toWrite;

                                if (expectedChunkSize == 0) removeCLRFAfterChunk = true;
                                if (responseData.empty()) break;
                            }
                            else
                            {
                                if (removeCLRFAfterChunk)
                                {
                                    if (responseData.size() >= 2)
                                    {
                                        removeCLRFAfterChunk = false;
                                        responseData.erase(responseData.begin(), responseData.begin() + 2);
                                    }
                                    else break;
                                }

                                auto i = std::search(responseData.begin(), responseData.end(), clrf.begin(), clrf.end());

                                if (i == responseData.end()) break;

                                std::string line(responseData.begin(), i);
                                responseData.erase(responseData.begin(), i + 2);

                                expectedChunkSize = std::stoul(line, 0, 16);

                                if (expectedChunkSize == 0)
                                {
                                    dataReceived = true;
                                    break;
                                }
                            }
                        }

                        if (dataReceived)
                            break;
                    }
                    else
                    {
                        response.body.insert(response.body.end(), responseData.begin(), responseData.end());
                        responseData.clear();

                        // got the whole content
                        if (contentSize == -1 || response.body.size() >= static_cast<size_t>(contentSize))
                            break;
                    }
                }
            }
            while (size > 0);

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
