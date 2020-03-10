//
//  HTTPRequest
//

#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <algorithm>
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <type_traits>
#include <vector>

#ifdef _WIN32
#  pragma push_macro("WIN32_LEAN_AND_MEAN")
#  pragma push_macro("NOMINMAX")
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  pragma pop_macro("WIN32_LEAN_AND_MEAN")
#  pragma pop_macro("NOMINMAX")
#else
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <netdb.h>
#  include <unistd.h>
#  include <errno.h>
#endif

namespace http
{
    class RequestError: public std::logic_error
    {
    public:
        explicit RequestError(const char* str): std::logic_error(str) {}
        explicit RequestError(const std::string& str): std::logic_error(str) {}
    };

    class ResponseError: public std::runtime_error
    {
    public:
        explicit ResponseError(const char* str): std::runtime_error(str) {}
        explicit ResponseError(const std::string& str): std::runtime_error(str) {}
    };


    enum class InternetProtocol: std::uint8_t
    {
        V4,
        V6
    };

    inline namespace detail
    {
#ifdef _WIN32
        class WinSock final
        {
        public:
            WinSock()
            {
                WSADATA wsaData;
                const int error = WSAStartup(MAKEWORD(2, 2), &wsaData);
                if (error != 0)
                    throw std::system_error(error, std::system_category(), "WSAStartup failed");

                if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
                {
                    WSACleanup();
                    throw std::runtime_error("Invalid WinSock version");
                }

                started = true;
            }

            ~WinSock()
            {
                if (started) WSACleanup();
            }

            WinSock(const WinSock&) = delete;
            WinSock& operator=(const WinSock&) = delete;

            WinSock(WinSock&& other) noexcept:
                started(other.started)
            {
                other.started = false;
            }

            WinSock& operator=(WinSock&& other) noexcept
            {
                if (&other == this) return *this;
                if (started) WSACleanup();
                started = other.started;
                other.started = false;
                return *this;
            }

        private:
            bool started = false;
        };
#endif

        inline int getLastError() noexcept
        {
#ifdef _WIN32
            return WSAGetLastError();
#else
            return errno;
#endif
        }

        constexpr int getAddressFamily(InternetProtocol internetProtocol)
        {
            return (internetProtocol == InternetProtocol::V4) ? AF_INET :
                (internetProtocol == InternetProtocol::V6) ? AF_INET6 :
                throw RequestError("Unsupported protocol");
        }
    }

    class Socket final
    {
    public:
#ifdef _WIN32
        using Type = SOCKET;
        static constexpr Type invalid = INVALID_SOCKET;
#else
        using Type = int;
        static constexpr Type invalid = -1;
#endif

        explicit Socket(InternetProtocol internetProtocol):
            endpoint(socket(getAddressFamily(internetProtocol), SOCK_STREAM, IPPROTO_TCP))
        {
            if (endpoint == invalid)
                throw std::system_error(getLastError(), std::system_category(), "Failed to create socket");
        }

        explicit Socket(Type s) noexcept:
            endpoint(s)
        {
        }

        ~Socket()
        {
            if (endpoint != invalid) close();
        }

        Socket(const Socket&) = delete;
        Socket& operator=(const Socket&) = delete;

        Socket(Socket&& other) noexcept:
            endpoint(other.endpoint)
        {
            other.endpoint = invalid;
        }

        Socket& operator=(Socket&& other) noexcept
        {
            if (&other == this) return *this;
            if (endpoint != invalid) close();
            endpoint = other.endpoint;
            other.endpoint = invalid;
            return *this;
        }

        inline operator Type() const noexcept { return endpoint; }

    private:
        inline void close() noexcept
        {
#ifdef _WIN32
            closesocket(endpoint);
#else
            ::close(endpoint);
#endif
        }

        Type endpoint = invalid;
    };

    inline std::string urlEncode(const std::string& str)
    {
        constexpr char hexChars[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

        std::string result;

        for (auto i = str.begin(); i != str.end(); ++i)
        {
            const std::uint8_t cp = *i & 0xFF;

            if ((cp >= 0x30 && cp <= 0x39) || // 0-9
                (cp >= 0x41 && cp <= 0x5A) || // A-Z
                (cp >= 0x61 && cp <= 0x7A) || // a-z
                cp == 0x2D || cp == 0x2E || cp == 0x5F) // - . _
                result += static_cast<char>(cp);
            else if (cp <= 0x7F) // length = 1
                result += std::string("%") + hexChars[(*i & 0xF0) >> 4] + hexChars[*i & 0x0F];
            else if ((cp >> 5) == 0x06) // length = 2
            {
                result += std::string("%") + hexChars[(*i & 0xF0) >> 4] + hexChars[*i & 0x0F];
                if (++i == str.end()) break;
                result += std::string("%") + hexChars[(*i & 0xF0) >> 4] + hexChars[*i & 0x0F];
            }
            else if ((cp >> 4) == 0x0E) // length = 3
            {
                result += std::string("%") + hexChars[(*i & 0xF0) >> 4] + hexChars[*i & 0x0F];
                if (++i == str.end()) break;
                result += std::string("%") + hexChars[(*i & 0xF0) >> 4] + hexChars[*i & 0x0F];
                if (++i == str.end()) break;
                result += std::string("%") + hexChars[(*i & 0xF0) >> 4] + hexChars[*i & 0x0F];
            }
            else if ((cp >> 3) == 0x1E) // length = 4
            {
                result += std::string("%") + hexChars[(*i & 0xF0) >> 4] + hexChars[*i & 0x0F];
                if (++i == str.end()) break;
                result += std::string("%") + hexChars[(*i & 0xF0) >> 4] + hexChars[*i & 0x0F];
                if (++i == str.end()) break;
                result += std::string("%") + hexChars[(*i & 0xF0) >> 4] + hexChars[*i & 0x0F];
                if (++i == str.end()) break;
                result += std::string("%") + hexChars[(*i & 0xF0) >> 4] + hexChars[*i & 0x0F];
            }
        }

        return result;
    }

    struct Response final
    {
        enum Status
        {
            STATUS_CONTINUE = 100,
            STATUS_SWITCHINGPROTOCOLS = 101,
            STATUS_PROCESSING = 102,
            STATUS_EARLYHINTS = 103,

            STATUS_OK = 200,
            STATUS_CREATED = 201,
            STATUS_ACCEPTED = 202,
            STATUS_NONAUTHORITATIVEINFORMATION = 203,
            STATUS_NOCONTENT = 204,
            STATUS_RESETCONTENT = 205,
            STATUS_PARTIALCONTENT = 206,
            STATUS_MULTISTATUS = 207,
            STATUS_ALREADYREPORTED = 208,
            STATUS_IMUSED = 226,

            STATUS_MULTIPLECHOICES = 300,
            STATUS_MOVEDPERMANENTLY = 301,
            STATUS_FOUND = 302,
            STATUS_SEEOTHER = 303,
            STATUS_NOTMODIFIED = 304,
            STATUS_USEPROXY = 305,
            STATUS_TEMPORARYREDIRECT = 307,
            STATUS_PERMANENTREDIRECT = 308,

            STATUS_BADREQUEST = 400,
            STATUS_UNAUTHORIZED = 401,
            STATUS_PAYMENTREQUIRED = 402,
            STATUS_FORBIDDEN = 403,
            STATUS_NOTFOUND = 404,
            STATUS_METHODNOTALLOWED = 405,
            STATUS_NOTACCEPTABLE = 406,
            STATUS_PROXYAUTHENTICATIONREQUIRED = 407,
            STATUS_REQUESTTIMEOUT = 408,
            STATUS_CONFLICT = 409,
            STATUS_GONE = 410,
            STATUS_LENGTHREQUIRED = 411,
            STATUS_PRECONDITIONFAILED = 412,
            STATUS_PAYLOADTOOLARGE = 413,
            STATUS_URITOOLONG = 414,
            STATUS_UNSUPPORTEDMEDIATYPE = 415,
            STATUS_RANGENOTSATISFIABLE = 416,
            STATUS_EXPECTATIONFAILED = 417,
            STATUS_IMATEAPOT = 418,
            STATUS_MISDIRECTEDREQUEST = 421,
            STATUS_UNPROCESSABLEENTITY = 422,
            STATUS_LOCKED = 423,
            STATUS_FAILEDDEPENDENCY = 424,
            STATUS_TOOEARLY = 425,
            STATUS_UPGRADEREQUIRED = 426,
            STATUS_PRECONDITIONREQUIRED = 428,
            STATUS_TOOMANYREQUESTS = 429,
            STATUS_REQUESTHEADERFIELDSTOOLARGE = 431,
            STATUS_UNAVAILABLEFORLEGALREASONS = 451,

            STATUS_INTERNALSERVERERROR = 500,
            STATUS_NOTIMPLEMENTED = 501,
            STATUS_BADGATEWAY = 502,
            STATUS_SERVICEUNAVAILABLE = 503,
            STATUS_GATEWAYTIMEOUT = 504,
            STATUS_HTTPVERSIONNOTSUPPORTED = 505,
            STATUS_VARIANTALSONEGOTIATES = 506,
            STATUS_INSUFFICIENTSTORAGE = 507,
            STATUS_LOOPDETECTED = 508,
            STATUS_NOTEXTENDED = 510,
            STATUS_NETWORKAUTHENTICATIONREQUIRED = 511
        };

        int status = 0;
        std::vector<std::string> headers;
        std::vector<std::uint8_t> body;
    };

    class Request final
    {
    public:
        explicit Request(const std::string& url,
                         InternetProtocol protocol = InternetProtocol::V4):
            internetProtocol(protocol)
        {
            const auto schemeEndPosition = url.find("://");

            if (schemeEndPosition != std::string::npos)
            {
                scheme = url.substr(0, schemeEndPosition);
                path = url.substr(schemeEndPosition + 3);
            }
            else
            {
                scheme = "http";
                path = url;
            }

            const auto fragmentPosition = path.find('#');

            // remove the fragment part
            if (fragmentPosition != std::string::npos)
                path.resize(fragmentPosition);

            const auto pathPosition = path.find('/');

            if (pathPosition == std::string::npos)
            {
                domain = path;
                path = "/";
            }
            else
            {
                domain = path.substr(0, pathPosition);
                path = path.substr(pathPosition);
            }

            const auto portPosition = domain.find(':');

            if (portPosition != std::string::npos)
            {
                port = domain.substr(portPosition + 1);
                domain.resize(portPosition);
            }
            else
                port = "80";
        }

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

        Response send(const std::string& method = "GET",
                      const std::string& body = "",
                      const std::vector<std::string>& headers = {})
        {
            Response response;

            if (scheme != "http")
                throw RequestError("Only HTTP scheme is supported");

            addrinfo hints = {};
            hints.ai_family = getAddressFamily(internetProtocol);
            hints.ai_socktype = SOCK_STREAM;

            addrinfo* info;
            if (getaddrinfo(domain.c_str(), port.c_str(), &hints, &info) != 0)
                throw std::system_error(getLastError(), std::system_category(), "Failed to get address info of " + domain);

            std::unique_ptr<addrinfo, decltype(&freeaddrinfo)> addressInfo(info, freeaddrinfo);

            Socket socket(internetProtocol);
			
            // take the first address from the list
            if (::connect(socket, addressInfo->ai_addr, static_cast<socklen_t>(addressInfo->ai_addrlen)) < 0)
                throw std::system_error(getLastError(), std::system_category(), "Failed to connect to " + domain + ":" + port);

            std::string requestData = method + " " + path + " HTTP/1.1\r\n";

            for (const std::string& header : headers)
                requestData += header + "\r\n";

            requestData += "Host: " + domain + "\r\n";
            requestData += "Content-Length: " + std::to_string(body.size()) + "\r\n";

            requestData += "\r\n";
            requestData += body;

#if defined(__APPLE__) || defined(_WIN32)
            constexpr int flags = 0;
#else
            constexpr int flags = MSG_NOSIGNAL;
#endif

            using SizeType = decltype(::send(int{}, nullptr, size_t{}, int{}));
            auto remaining = static_cast<SizeType>(requestData.size());
            SizeType sent = 0;

            // send the request
            while (remaining > 0)
            {
                const auto size = ::send(socket, requestData.data() + sent, static_cast<size_t>(remaining), flags);

                if (size < 0)
                    throw std::system_error(getLastError(), std::system_category(), "Failed to send data to " + domain + ":" + port);

                remaining -= size;
                sent += size;
            }

            std::uint8_t tempBuffer[4096];
            constexpr std::uint8_t crlf[] = {'\r', '\n'};
            std::vector<std::uint8_t> responseData;
            bool firstLine = true;
            bool parsedHeaders = false;
            bool contentLengthReceived = false;
            unsigned long contentLength = 0;
            bool chunkedResponse = false;
            std::size_t expectedChunkSize = 0;
            bool removeCrlfAfterChunk = false;

            // read the response
            for (;;)
            {
                const auto size = recv(socket, reinterpret_cast<char*>(tempBuffer), sizeof(tempBuffer), flags);

                if (size < 0)
                    throw std::system_error(getLastError(), std::system_category(), "Failed to read data from " + domain + ":" + port);
                else if (size == 0)
                    break; // disconnected

                responseData.insert(responseData.end(), tempBuffer, tempBuffer + size);

                if (!parsedHeaders)
                {
                    for (;;)
                    {
                        const auto i = std::search(responseData.begin(), responseData.end(), std::begin(crlf), std::end(crlf));

                        // didn't find a newline
                        if (i == responseData.end()) break;

                        const std::string line(responseData.begin(), i);
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

                            std::string::size_type lastPos = 0;
                            const auto length = line.length();
                            std::vector<std::string> parts;

                            // tokenize first line
                            while (lastPos < length + 1)
                            {
                                auto pos = line.find(' ', lastPos);
                                if (pos == std::string::npos) pos = length;

                                if (pos != lastPos)
                                    parts.emplace_back(line.data() + lastPos,
                                                       static_cast<std::vector<std::string>::size_type>(pos) - lastPos);

                                lastPos = pos + 1;
                            }

                            if (parts.size() >= 2)
                                response.status = std::stoi(parts[1]);
                        }
                        else // headers
                        {
                            response.headers.push_back(line);

                            const auto pos = line.find(':');

                            if (pos != std::string::npos)
                            {
                                std::string headerName = line.substr(0, pos);
                                std::string headerValue = line.substr(pos + 1);

                                // ltrim
                                headerValue.erase(headerValue.begin(),
                                                  std::find_if(headerValue.begin(), headerValue.end(),
                                                               [](int c) {return !std::isspace(c);}));

                                // rtrim
                                headerValue.erase(std::find_if(headerValue.rbegin(), headerValue.rend(),
                                                               [](int c) {return !std::isspace(c);}).base(),
                                                  headerValue.end());

                                if (headerName == "Content-Length")
                                {
                                    contentLength = std::stoul(headerValue);
                                    contentLengthReceived = true;
                                    response.body.reserve(contentLength);
                                }
                                else if (headerName == "Transfer-Encoding")
                                {
                                    if (headerValue == "chunked")
                                        chunkedResponse = true;
                                    else
                                        throw ResponseError("Unsupported transfer encoding: " + headerValue);
                                }
                            }
                        }
                    }
                }

                if (parsedHeaders)
                {
                    // Content-Length must be ignored if Transfer-Encoding is received
                    if (chunkedResponse)
                    {
                        bool dataReceived = false;
                        for (;;)
                        {
                            if (expectedChunkSize > 0)
                            {
                                const auto toWrite = std::min(expectedChunkSize, responseData.size());
                                response.body.insert(response.body.end(), responseData.begin(), responseData.begin() + static_cast<ptrdiff_t>(toWrite));
                                responseData.erase(responseData.begin(), responseData.begin() + static_cast<ptrdiff_t>(toWrite));
                                expectedChunkSize -= toWrite;

                                if (expectedChunkSize == 0) removeCrlfAfterChunk = true;
                                if (responseData.empty()) break;
                            }
                            else
                            {
                                if (removeCrlfAfterChunk)
                                {
                                    if (responseData.size() >= 2)
                                    {
                                        removeCrlfAfterChunk = false;
                                        responseData.erase(responseData.begin(), responseData.begin() + 2);
                                    }
                                    else break;
                                }

                                const auto i = std::search(responseData.begin(), responseData.end(), std::begin(crlf), std::end(crlf));

                                if (i == responseData.end()) break;

                                const std::string line(responseData.begin(), i);
                                responseData.erase(responseData.begin(), i + 2);

                                expectedChunkSize = std::stoul(line, nullptr, 16);

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
                        if (contentLengthReceived && response.body.size() >= contentLength)
                            break;
                    }
                }
            }

            return response;
        }

    private:
#ifdef _WIN32
        WinSock winSock;
#endif
        InternetProtocol internetProtocol;
        std::string scheme;
        std::string domain;
        std::string port;
        std::string path;
    };
}

#endif
