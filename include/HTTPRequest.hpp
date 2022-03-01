//
//  HTTPRequest
//

#ifndef HTTPREQUEST_HPP
#define HTTPREQUEST_HPP

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <array>
#include <chrono>
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <system_error>
#include <type_traits>
#include <vector>

#if defined(_WIN32) || defined(__CYGWIN__)
#  pragma push_macro("WIN32_LEAN_AND_MEAN")
#  pragma push_macro("NOMINMAX")
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif // WIN32_LEAN_AND_MEAN
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif // NOMINMAX
#  include <winsock2.h>
#  if _WIN32_WINNT < _WIN32_WINNT_WINXP
extern "C" char *_strdup(const char *strSource);
#    define strdup _strdup
#    include <wspiapi.h>
#  endif // _WIN32_WINNT < _WIN32_WINNT_WINXP
#  include <ws2tcpip.h>
#  pragma pop_macro("WIN32_LEAN_AND_MEAN")
#  pragma pop_macro("NOMINMAX")
#else
#  include <errno.h>
#  include <fcntl.h>
#  include <netinet/in.h>
#  include <netdb.h>
#  include <sys/select.h>
#  include <sys/socket.h>
#  include <sys/types.h>
#  include <unistd.h>
#endif // defined(_WIN32) || defined(__CYGWIN__)

namespace http
{
    class RequestError final: public std::logic_error
    {
    public:
        explicit RequestError(const char* str): std::logic_error{str} {}
        explicit RequestError(const std::string& str): std::logic_error{str} {}
    };

    class ResponseError final: public std::runtime_error
    {
    public:
        explicit ResponseError(const char* str): std::runtime_error{str} {}
        explicit ResponseError(const std::string& str): std::runtime_error{str} {}
    };

    enum class InternetProtocol: std::uint8_t
    {
        V4,
        V6
    };

    struct Uri final
    {
        std::string scheme;
        std::string authority;
        std::string userinfo;
        std::string user;
        std::string password;
        std::string host;
        std::string port;
        std::string path;
        std::string query;
        std::string fragment;
    };

    struct HttpVersion final
    {
        uint16_t major;
        uint16_t minor;
    };

    struct Status final
    {
        // RFC 7231, 6. Response Status Codes
        enum Code: std::uint16_t
        {
            Continue = 100,
            SwitchingProtocol = 101,
            Processing = 102,
            EarlyHints = 103,

            Ok = 200,
            Created = 201,
            Accepted = 202,
            NonAuthoritativeInformation = 203,
            NoContent = 204,
            ResetContent = 205,
            PartialContent = 206,
            MultiStatus = 207,
            AlreadyReported = 208,
            ImUsed = 226,

            MultipleChoice = 300,
            MovedPermanently = 301,
            Found = 302,
            SeeOther = 303,
            NotModified = 304,
            UseProxy = 305,
            TemporaryRedirect = 307,
            PermanentRedirect = 308,

            BadRequest = 400,
            Unauthorized = 401,
            PaymentRequired = 402,
            Forbidden = 403,
            NotFound = 404,
            MethodNotAllowed = 405,
            NotAcceptable = 406,
            ProxyAuthenticationRequired = 407,
            RequestTimeout = 408,
            Conflict = 409,
            Gone = 410,
            LengthRequired = 411,
            PreconditionFailed = 412,
            PayloadTooLarge = 413,
            UriTooLong = 414,
            UnsupportedMediaType = 415,
            RangeNotSatisfiable = 416,
            ExpectationFailed = 417,
            MisdirectedRequest = 421,
            UnprocessableEntity = 422,
            Locked = 423,
            FailedDependency = 424,
            TooEarly = 425,
            UpgradeRequired = 426,
            PreconditionRequired = 428,
            TooManyRequests = 429,
            RequestHeaderFieldsTooLarge = 431,
            UnavailableForLegalReasons = 451,

            InternalServerError = 500,
            NotImplemented = 501,
            BadGateway = 502,
            ServiceUnavailable = 503,
            GatewayTimeout = 504,
            HttpVersionNotSupported = 505,
            VariantAlsoNegotiates = 506,
            InsufficientStorage = 507,
            LoopDetected = 508,
            NotExtended = 510,
            NetworkAuthenticationRequired = 511
        };

        HttpVersion httpVersion;
        std::uint16_t code;
        std::string reason;
    };

    struct Response final
    {
        Status status;
        std::vector<std::pair<std::string, std::string>> headers;
        std::vector<std::uint8_t> body;
    };

    inline namespace detail
    {
#if defined(_WIN32) || defined(__CYGWIN__)
        class WinSock final
        {
        public:
            WinSock()
            {
                WSADATA wsaData;
                const auto error = WSAStartup(MAKEWORD(2, 2), &wsaData);
                if (error != 0)
                    throw std::system_error{error, std::system_category(), "WSAStartup failed"};

                if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
                {
                    WSACleanup();
                    throw std::runtime_error{"Invalid WinSock version"};
                }

                started = true;
            }

            ~WinSock()
            {
                if (started) WSACleanup();
            }

            WinSock(WinSock&& other) noexcept:
                started{other.started}
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
#endif // defined(_WIN32) || defined(__CYGWIN__)

        inline int getLastError() noexcept
        {
#if defined(_WIN32) || defined(__CYGWIN__)
            return WSAGetLastError();
#else
            return errno;
#endif // defined(_WIN32) || defined(__CYGWIN__)
        }

        constexpr int getAddressFamily(const InternetProtocol internetProtocol)
        {
            return (internetProtocol == InternetProtocol::V4) ? AF_INET :
                (internetProtocol == InternetProtocol::V6) ? AF_INET6 :
                throw RequestError{"Unsupported protocol"};
        }

        class Socket final
        {
        public:
#if defined(_WIN32) || defined(__CYGWIN__)
            using Type = SOCKET;
            static constexpr Type invalid = INVALID_SOCKET;
#else
            using Type = int;
            static constexpr Type invalid = -1;
#endif // defined(_WIN32) || defined(__CYGWIN__)

            explicit Socket(const InternetProtocol internetProtocol):
                endpoint{socket(getAddressFamily(internetProtocol), SOCK_STREAM, IPPROTO_TCP)}
            {
                if (endpoint == invalid)
                    throw std::system_error{getLastError(), std::system_category(), "Failed to create socket"};

#if defined(_WIN32) || defined(__CYGWIN__)
                ULONG mode = 1;
                if (ioctlsocket(endpoint, FIONBIO, &mode) != 0)
                {
                    close();
                    throw std::system_error{WSAGetLastError(), std::system_category(), "Failed to get socket flags"};
                }
#else
                const auto flags = fcntl(endpoint, F_GETFL);
                if (flags == -1)
                {
                    close();
                    throw std::system_error{errno, std::system_category(), "Failed to get socket flags"};
                }

                if (fcntl(endpoint, F_SETFL, flags | O_NONBLOCK) == -1)
                {
                    close();
                    throw std::system_error{errno, std::system_category(), "Failed to set socket flags"};
                }
#endif // defined(_WIN32) || defined(__CYGWIN__)

#ifdef __APPLE__
                const int value = 1;
                if (setsockopt(endpoint, SOL_SOCKET, SO_NOSIGPIPE, &value, sizeof(value)) == -1)
                {
                    close();
                    throw std::system_error{errno, std::system_category(), "Failed to set socket option"};
                }
#endif // __APPLE__
            }

            ~Socket()
            {
                if (endpoint != invalid) close();
            }

            Socket(Socket&& other) noexcept:
                endpoint{other.endpoint}
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

            void connect(const struct sockaddr* address, const socklen_t addressSize, const std::int64_t timeout)
            {
#if defined(_WIN32) || defined(__CYGWIN__)
                auto result = ::connect(endpoint, address, addressSize);
                while (result == -1 && WSAGetLastError() == WSAEINTR)
                    result = ::connect(endpoint, address, addressSize);

                if (result == -1)
                {
                    if (WSAGetLastError() == WSAEWOULDBLOCK)
                    {
                        select(SelectType::write, timeout);

                        char socketErrorPointer[sizeof(int)];
                        socklen_t optionLength = sizeof(socketErrorPointer);
                        if (getsockopt(endpoint, SOL_SOCKET, SO_ERROR, socketErrorPointer, &optionLength) == -1)
                            throw std::system_error{WSAGetLastError(), std::system_category(), "Failed to get socket option"};

                        int socketError;
                        std::memcpy(&socketError, socketErrorPointer, sizeof(socketErrorPointer));

                        if (socketError != 0)
                            throw std::system_error{socketError, std::system_category(), "Failed to connect"};
                    }
                    else
                        throw std::system_error{WSAGetLastError(), std::system_category(), "Failed to connect"};
                }
#else
                auto result = ::connect(endpoint, address, addressSize);
                while (result == -1 && errno == EINTR)
                    result = ::connect(endpoint, address, addressSize);

                if (result == -1)
                {
                    if (errno == EINPROGRESS)
                    {
                        select(SelectType::write, timeout);

                        int socketError;
                        socklen_t optionLength = sizeof(socketError);
                        if (getsockopt(endpoint, SOL_SOCKET, SO_ERROR, &socketError, &optionLength) == -1)
                            throw std::system_error{errno, std::system_category(), "Failed to get socket option"};

                        if (socketError != 0)
                            throw std::system_error{socketError, std::system_category(), "Failed to connect"};
                    }
                    else
                        throw std::system_error{errno, std::system_category(), "Failed to connect"};
                }
#endif // defined(_WIN32) || defined(__CYGWIN__)
            }

            std::size_t send(const void* buffer, const std::size_t length, const std::int64_t timeout)
            {
                select(SelectType::write, timeout);
#if defined(_WIN32) || defined(__CYGWIN__)
                auto result = ::send(endpoint, reinterpret_cast<const char*>(buffer),
                                     static_cast<int>(length), 0);

                while (result == -1 && WSAGetLastError() == WSAEINTR)
                    result = ::send(endpoint, reinterpret_cast<const char*>(buffer),
                                    static_cast<int>(length), 0);

                if (result == -1)
                    throw std::system_error{WSAGetLastError(), std::system_category(), "Failed to send data"};
#else
                auto result = ::send(endpoint, reinterpret_cast<const char*>(buffer),
                                     length, noSignal);

                while (result == -1 && errno == EINTR)
                    result = ::send(endpoint, reinterpret_cast<const char*>(buffer),
                                    length, noSignal);

                if (result == -1)
                    throw std::system_error{errno, std::system_category(), "Failed to send data"};
#endif // defined(_WIN32) || defined(__CYGWIN__)
                return static_cast<std::size_t>(result);
            }

            std::size_t recv(void* buffer, const std::size_t length, const std::int64_t timeout)
            {
                select(SelectType::read, timeout);
#if defined(_WIN32) || defined(__CYGWIN__)
                auto result = ::recv(endpoint, reinterpret_cast<char*>(buffer),
                                     static_cast<int>(length), 0);

                while (result == -1 && WSAGetLastError() == WSAEINTR)
                    result = ::recv(endpoint, reinterpret_cast<char*>(buffer),
                                    static_cast<int>(length), 0);

                if (result == -1)
                    throw std::system_error{WSAGetLastError(), std::system_category(), "Failed to read data"};
#else
                auto result = ::recv(endpoint, reinterpret_cast<char*>(buffer),
                                     length, noSignal);

                while (result == -1 && errno == EINTR)
                    result = ::recv(endpoint, reinterpret_cast<char*>(buffer),
                                    length, noSignal);

                if (result == -1)
                    throw std::system_error{errno, std::system_category(), "Failed to read data"};
#endif // defined(_WIN32) || defined(__CYGWIN__)
                return static_cast<std::size_t>(result);
            }

        private:
            enum class SelectType
            {
                read,
                write
            };

            void select(const SelectType type, const std::int64_t timeout)
            {
                fd_set descriptorSet;
                FD_ZERO(&descriptorSet);
                FD_SET(endpoint, &descriptorSet);

#if defined(_WIN32) || defined(__CYGWIN__)
                TIMEVAL selectTimeout{
                    static_cast<LONG>(timeout / 1000),
                    static_cast<LONG>((timeout % 1000) * 1000)
                };
                auto count = ::select(0,
                                      (type == SelectType::read) ? &descriptorSet : nullptr,
                                      (type == SelectType::write) ? &descriptorSet : nullptr,
                                      nullptr,
                                      (timeout >= 0) ? &selectTimeout : nullptr);

                while (count == -1 && WSAGetLastError() == WSAEINTR)
                    count = ::select(0,
                                     (type == SelectType::read) ? &descriptorSet : nullptr,
                                     (type == SelectType::write) ? &descriptorSet : nullptr,
                                     nullptr,
                                     (timeout >= 0) ? &selectTimeout : nullptr);

                if (count == -1)
                    throw std::system_error{WSAGetLastError(), std::system_category(), "Failed to select socket"};
                else if (count == 0)
                    throw ResponseError{"Request timed out"};
#else
                timeval selectTimeout{
                    static_cast<time_t>(timeout / 1000),
                    static_cast<suseconds_t>((timeout % 1000) * 1000)
                };
                auto count = ::select(endpoint + 1,
                                      (type == SelectType::read) ? &descriptorSet : nullptr,
                                      (type == SelectType::write) ? &descriptorSet : nullptr,
                                      nullptr,
                                      (timeout >= 0) ? &selectTimeout : nullptr);

                while (count == -1 && errno == EINTR)
                    count = ::select(endpoint + 1,
                                     (type == SelectType::read) ? &descriptorSet : nullptr,
                                     (type == SelectType::write) ? &descriptorSet : nullptr,
                                     nullptr,
                                     (timeout >= 0) ? &selectTimeout : nullptr);

                if (count == -1)
                    throw std::system_error{errno, std::system_category(), "Failed to select socket"};
                else if (count == 0)
                    throw ResponseError{"Request timed out"};
#endif // defined(_WIN32) || defined(__CYGWIN__)
            }

            void close() noexcept
            {
#if defined(_WIN32) || defined(__CYGWIN__)
                closesocket(endpoint);
#else
                ::close(endpoint);
#endif // defined(_WIN32) || defined(__CYGWIN__)
            }

#if defined(__unix__) && !defined(__APPLE__) && !defined(__CYGWIN__)
            static constexpr int noSignal = MSG_NOSIGNAL;
#else
            static constexpr int noSignal = 0;
#endif // defined(__unix__) && !defined(__APPLE__)

            Type endpoint = invalid;
        };

        inline bool isWhitespaceChar(const char c) noexcept
        {
            // RFC 7230, 3.2.3. Whitespace
            return c == ' ' || c == '\t';
        };

        inline bool isDigitChar(const char c) noexcept
        {
            // RFC 5234, Appendix B.1. Core Rules
            return c >= '0' && c <= '9';
        }

        inline bool isAlphaChar(const char c) noexcept
        {
            // RFC 5234, Appendix B.1. Core Rules
            return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
        }

        inline bool isTokenChar(const char c) noexcept
        {
            // RFC 7230, 3.2.6. Field Value Components
            return c == '!' || c == '#' || c == '$' || c == '%' || c == '&' || c == '\'' || c == '*' ||
                c == '+' || c == '-' || c == '.' || c == '^' || c == '_' || c == '`' || c == '|' || c == '~' ||
                isDigitChar(c) ||
                isAlphaChar(c);
        };

        // RFC 5234, Appendix B.1. Core Rules
        inline bool isVisibleChar(const char c) noexcept
        {
            return c >= 0x21 && c <= 0x7E;
        }

        // RFC 7230, Appendix B. Collected ABNF
        inline bool isObsoleteTextChar(const char c) noexcept
        {
            return static_cast<unsigned char>(c) >= 0x80 &&
                static_cast<unsigned char>(c) <= 0xFF;
        }

        template <class Iterator>
        Iterator skipWhitespaces(const Iterator begin, const Iterator end)
        {
            auto i = begin;
            for (i = begin; i != end; ++i)
                if (!isWhitespaceChar(*i))
                    break;

            return i;
        }

        // RFC 3986, 3. Syntax Components
        template <class Iterator>
        Uri parseUri(const Iterator begin, const Iterator end)
        {
            std::string schemeEnd = "://";

            Uri result;

            const auto schemeEndIterator = std::search(begin, end, schemeEnd.begin(), schemeEnd.end());
            if (schemeEndIterator != end)
            {
                result.scheme = std::string(begin, schemeEndIterator);
                result.authority = std::string(schemeEndIterator + 3, end);
            }
            else
                throw RequestError{"Invalid URI"};

            // remove the fragment part
            const auto fragmentPosition = result.authority.find('#');
            if (fragmentPosition != std::string::npos)
            {
                result.fragment = result.authority.substr(fragmentPosition + 1);
                result.authority.resize(fragmentPosition);
            }

            // remove the query part
            const auto queryPosition = result.authority.find('?');
            if (queryPosition != std::string::npos)
            {
                result.query = result.authority.substr(queryPosition + 1);
                result.authority.resize(queryPosition);
            }

            const auto pathPosition = result.authority.find('/');
            if (pathPosition != std::string::npos)
            {
                result.path = result.authority.substr(pathPosition);
                result.authority = result.authority.substr(0, pathPosition);
            }
            else
            {
                result.path = "/";
            }

            const auto hostPosition = result.authority.find('@');
            if (hostPosition != std::string::npos)
            {
                result.userinfo = result.authority.substr(0, hostPosition);

                const auto passwordPosition = result.userinfo.find(':');
                if (passwordPosition != std::string::npos)
                {
                    result.user = result.userinfo.substr(0, passwordPosition);
                    result.password = result.userinfo.substr(passwordPosition + 1);
                }
                else
                    result.user = result.userinfo;

                result.host = result.authority.substr(hostPosition + 1);
            }
            else
                result.host = result.authority;

            const auto portPosition = result.host.find(':');
            if (portPosition != std::string::npos)
            {
                result.port = result.host.substr(portPosition + 1);
                result.host = result.host.substr(0, portPosition);
            }
            else
                result.host = result.host;

            return result;
        }

        // RFC 7230, 2.6. Protocol Versioning
        template <class Iterator>
        std::pair<Iterator, HttpVersion> parseHttpVersion(const Iterator begin, const Iterator end)
        {
            auto i = begin;

            constexpr std::array<char, 4> http{'H', 'T', 'T', 'P'};

            for (std::size_t n = 0; n < http.size(); ++n, ++i)
                if (i == end || *i != http[n])
                    throw ResponseError{"Invalid HTTP version"};

            if (i == end || *i != '/')
                throw ResponseError{"Invalid HTTP version"};

            ++i;

            if (i == end || !isDigitChar(*i))
                throw ResponseError{"Invalid HTTP version"};

            const auto majorVersion = static_cast<uint16_t>(*i - '0');

            ++i;

            if (i == end || *i != '.')
                throw ResponseError{"Invalid HTTP version"};

            ++i;

            if (i == end || !isDigitChar(*i))
                throw ResponseError{"Invalid HTTP version"};

            const auto minorVersion = static_cast<uint16_t>(*i - '0');

            ++i;

            return std::make_pair(i, HttpVersion{majorVersion, minorVersion});
        }

        // RFC 7230, 3.1.2. Status Line
        template <class Iterator>
        std::pair<Iterator, std::uint16_t> parseStatusCode(const Iterator begin, const Iterator end)
        {
            std::uint16_t result = 0;

            auto i = begin;
            for (std::size_t n = 0; n < 3; ++n, ++i)
                if (i == end || !isDigitChar(*i))
                    throw ResponseError{"Invalid status code"};
                else
                    result = static_cast<uint16_t>(result * 10U + static_cast<uint16_t>(*i - '0'));

            return std::make_pair(i, result);
        }

        // RFC 7230, 3.1.2. Status Line
        template <class Iterator>
        std::pair<Iterator, std::string> parseReasonPhrase(const Iterator begin, const Iterator end)
        {
            std::string result;

            auto i = begin;
            for (; i != end && (isWhitespaceChar(*i) || isVisibleChar(*i) || isObsoleteTextChar(*i)); ++i)
                result.push_back(*i);

            return std::make_pair(i, std::move(result));
        }

        // RFC 7230, 3.2.6. Field Value Components
        template <class Iterator>
        std::pair<Iterator, std::string> parseToken(const Iterator begin, const Iterator end)
        {
            std::string result;

            auto i = begin;
            for (; i != end && isTokenChar(*i); ++i)
                result.push_back(*i);

            if (result.empty())
                throw ResponseError{"Invalid token"};

            return std::make_pair(i, std::move(result));
        }

        // RFC 7230, 3.2. Header Fields
        template <class Iterator>
        std::pair<Iterator, std::string> parseFieldValue(const Iterator begin, const Iterator end)
        {
            std::string result;

            auto i = begin;
            for (; i != end && (isWhitespaceChar(*i) || isVisibleChar(*i) || isObsoleteTextChar(*i)); ++i)
                result.push_back(*i);

            result.erase(std::find_if(result.rbegin(), result.rend(), [](char c) {
                    return !isWhitespaceChar(c);
            }).base(), result.end());

            return std::make_pair(i, std::move(result));
        }

        // RFC 7230, 3.2. Header Fields
        template <class Iterator>
        std::pair<Iterator, std::string> parseFieldContent(const Iterator begin, const Iterator end)
        {
            std::string result;

            auto i = begin;

            for (;;)
            {
                const auto fieldValueResult = parseFieldValue(i, end);
                i = fieldValueResult.first;
                result += fieldValueResult.second;

                // Handle obsolete fold as per RFC 7230, 3.2.4. Field Parsing
                // Obsolete folding is known as linear white space (LWS) in RFC 2616, 2.2 Basic Rules
                auto obsoleteFoldIterator = i;
                if (obsoleteFoldIterator == end || *obsoleteFoldIterator++ != '\r')
                    break;

                if (obsoleteFoldIterator == end || *obsoleteFoldIterator++ != '\n')
                    break;

                if (obsoleteFoldIterator == end || !isWhitespaceChar(*obsoleteFoldIterator++))
                    break;

                result.push_back(' ');
                i = obsoleteFoldIterator;
            }

            return std::make_pair(i, std::move(result));;
        }

        // RFC 7230, 3.2. Header Fields
        template <class Iterator>
        std::pair<Iterator, std::pair<std::string, std::string>> parseHeaderField(const Iterator begin, const Iterator end)
        {
            const auto tokenResult = parseToken(begin, end);
            auto i = tokenResult.first;
            auto fieldName = std::move(tokenResult.second);

            if (i == end || *i++ != ':')
                throw ResponseError{"Invalid header"};

            i = skipWhitespaces(i, end);

            const auto valueResult = parseFieldContent(i, end);
            i = valueResult.first;
            auto fieldValue = std::move(valueResult.second);

            if (i == end || *i++ != '\r')
                throw ResponseError{"Invalid header"};

            if (i == end || *i++ != '\n')
                throw ResponseError{"Invalid header"};

            return std::make_pair(i, std::make_pair(std::move(fieldName), std::move(fieldValue)));
        }

        // RFC 7230, 3.1.2. Status Line
        template <class Iterator>
        std::pair<Iterator, Status> parseStatusLine(const Iterator begin, const Iterator end)
        {
            const auto httpVersionResult = parseHttpVersion(begin, end);
            auto i = httpVersionResult.first;

            if (i == end || *i++ != ' ')
                throw ResponseError{"Invalid status line"};

            const auto statusCodeResult = parseStatusCode(i, end);
            i = statusCodeResult.first;

            if (i == end || *i++ != ' ')
                throw ResponseError{"Invalid status line"};

            const auto reasonPhraseResult = parseReasonPhrase(i, end);
            i = reasonPhraseResult.first;

            if (i == end || *i++ != '\r')
                throw ResponseError{"Invalid status line"};

            if (i == end || *i++ != '\n')
                throw ResponseError{"Invalid status line"};

            return std::make_pair(i, Status{
                httpVersionResult.second,
                statusCodeResult.second,
                std::move(reasonPhraseResult.second)
            });
        }

        // RFC 7230, 3.1.1. Request Line
        inline std::string encodeRequestLine(const std::string& method, const std::string& target)
        {
            return method + " " + target + " HTTP/1.1\r\n";
        }

        // RFC 7230, 3.2. Header Fields
        inline std::string encodeHeaders(const std::vector<std::pair<std::string, std::string>>& headers)
        {
            std::string result;
            for (const auto& header : headers)
                result += header.first + ": " + header.second + "\r\n";
            return result;
        }

        // RFC 5234, Appendix B.1. Core Rules
        template <typename T, typename std::enable_if<std::is_unsigned<T>::value>::type* = nullptr>
        constexpr T hexToUint(char c)
        {
            return (c >= '0' && c <= '9') ? static_cast<T>(c - '0') :
                (c >= 'A' && c <= 'F') ? static_cast<T>(c - 'A') + T(10) :
                (c >= 'a' && c <= 'f') ? static_cast<T>(c - 'a') + T(10) : // some services send lower-case hex digits
                throw ResponseError{"Invalid hex integer"};
        }

        // RFC 7230, 4.1. Chunked Transfer Coding
        template <typename T, class Iterator, typename std::enable_if<std::is_unsigned<T>::value>::type* = nullptr>
        constexpr T hexToUint(const Iterator begin, const Iterator end, const T value = 0)
        {
            return begin == end ? value :
                hexToUint<T>(begin + 1, end, value * 16 + hexToUint<T>(*begin));
        }

        // RFC 4648, 4. Base 64 Encoding
        template <class Iterator>
        std::string encodeBase64(const Iterator begin, const Iterator end)
        {
            static constexpr std::array<char, 64> chars{
                'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
                'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
                'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
                'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
                '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'
            };

            std::string result;
            std::size_t c = 0;
            std::array<std::uint8_t, 3> charArray;

            for (auto i = begin; i != end; ++i)
            {
                charArray[c++] = static_cast<std::uint8_t>(*i);
                if (c == 3)
                {
                    result += chars[static_cast<std::uint8_t>((charArray[0] & 0xFC) >> 2)];
                    result += chars[static_cast<std::uint8_t>(((charArray[0] & 0x03) << 4) + ((charArray[1] & 0xF0) >> 4))];
                    result += chars[static_cast<std::uint8_t>(((charArray[1] & 0x0F) << 2) + ((charArray[2] & 0xC0) >> 6))];
                    result += chars[static_cast<std::uint8_t>(charArray[2] & 0x3f)];
                    c = 0;
                }
            }

            if (c)
            {
                result += chars[static_cast<std::uint8_t>((charArray[0] & 0xFC) >> 2)];

                if (c == 1)
                    result += chars[static_cast<std::uint8_t>((charArray[0] & 0x03) << 4)];
                else // c == 2
                {
                    result += chars[static_cast<std::uint8_t>(((charArray[0] & 0x03) << 4) + ((charArray[1] & 0xF0) >> 4))];
                    result += chars[static_cast<std::uint8_t>((charArray[1] & 0x0F) << 2)];
                }

                while (++c < 4) result += '='; // padding
            }

            return result;
        }
    }

    class Request final
    {
    public:
        explicit Request(const std::string& uri,
                         const InternetProtocol protocol = InternetProtocol::V4):
            internetProtocol{protocol},
            uri{parseUri(uri.begin(), uri.end())}
        {
        }

        Response send(const std::string& method = "GET",
                      const std::string& body = "",
                      const std::vector<std::pair<std::string, std::string>>& headers = {},
                      const std::chrono::milliseconds timeout = std::chrono::milliseconds{-1})
        {
            return send(method,
                        std::vector<uint8_t>(body.begin(), body.end()),
                        headers,
                        timeout);
        }

        Response send(const std::string& method,
                      const std::vector<uint8_t>& body,
                      std::vector<std::pair<std::string, std::string>> headers,
                      const std::chrono::milliseconds timeout = std::chrono::milliseconds{-1})
        {
            const auto stopTime = std::chrono::steady_clock::now() + timeout;

            if (uri.scheme != "http")
                throw RequestError{"Only HTTP scheme is supported"};

            addrinfo hints = {};
            hints.ai_family = getAddressFamily(internetProtocol);
            hints.ai_socktype = SOCK_STREAM;

            const char* port = uri.port.empty() ? "80" : uri.port.c_str();

            addrinfo* info;
            if (getaddrinfo(uri.host.c_str(), port, &hints, &info) != 0)
                throw std::system_error{getLastError(), std::system_category(), "Failed to get address info of " + uri.authority};

            const std::unique_ptr<addrinfo, decltype(&freeaddrinfo)> addressInfo{info, freeaddrinfo};

            // RFC 7230, 5.3. Request Target
            std::string requestTarget = uri.path + '?' + uri.query;

            // RFC 7230, 5.4. Host
            headers.push_back({"Host", uri.host});

            // RFC 7230, 3.3.2. Content-Length
            headers.push_back({"Content-Length", std::to_string(body.size())});

            // RFC 7617, 2. The 'Basic' Authentication Scheme
            if (!uri.userinfo.empty())
                headers.push_back({"Authorization", "Basic " + encodeBase64(uri.userinfo.begin(), uri.userinfo.end())});

            const std::string headerData = encodeRequestLine(method, requestTarget) +
                encodeHeaders(headers) +
                "\r\n";

            std::vector<uint8_t> requestData(headerData.begin(), headerData.end());
            requestData.insert(requestData.end(), body.begin(), body.end());

            Socket socket{internetProtocol};

            const auto getRemainingMilliseconds = [](const std::chrono::steady_clock::time_point time) noexcept -> std::int64_t {
                const auto now = std::chrono::steady_clock::now();
                const auto remainingTime = std::chrono::duration_cast<std::chrono::milliseconds>(time - now);
                return (remainingTime.count() > 0) ? remainingTime.count() : 0;
            };

            // take the first address from the list
            socket.connect(addressInfo->ai_addr, static_cast<socklen_t>(addressInfo->ai_addrlen),
                           (timeout.count() >= 0) ? getRemainingMilliseconds(stopTime) : -1);

            auto remaining = requestData.size();
            auto sendData = requestData.data();

            // send the request
            while (remaining > 0)
            {
                const auto size = socket.send(sendData, remaining,
                                              (timeout.count() >= 0) ? getRemainingMilliseconds(stopTime) : -1);
                remaining -= size;
                sendData += size;
            }

            std::array<std::uint8_t, 4096> tempBuffer;
            constexpr std::array<std::uint8_t, 2> crlf = {'\r', '\n'};
            constexpr std::array<std::uint8_t, 4> headerEnd = {'\r', '\n', '\r', '\n'};
            Response response;
            std::vector<std::uint8_t> responseData;
            bool parsingBody = false;
            bool contentLengthReceived = false;
            std::size_t contentLength = 0U;
            bool chunkedResponse = false;
            std::size_t expectedChunkSize = 0U;
            bool removeCrlfAfterChunk = false;

            // read the response
            for (;;)
            {
                const auto size = socket.recv(tempBuffer.data(), tempBuffer.size(),
                                              (timeout.count() >= 0) ? getRemainingMilliseconds(stopTime) : -1);
                if (size == 0) // disconnected
                    return response;

                responseData.insert(responseData.end(), tempBuffer.begin(), tempBuffer.begin() + size);

                if (!parsingBody)
                {
                    // RFC 7230, 3. Message Format
                    // Empty line indicates the end of the header section (RFC 7230, 2.1. Client/Server Messaging)
                    const auto headerEndIterator = std::search(responseData.cbegin(), responseData.cend(), headerEnd.cbegin(), headerEnd.cend());
                    if (headerEndIterator == responseData.cend()) break;

                    // didn't find a newline
                    const std::string headerResponseData(responseData.cbegin(), headerEndIterator + 2);
                    responseData.erase(responseData.cbegin(), headerEndIterator + 4);

                    const auto statusLineResult = parseStatusLine(headerResponseData.cbegin(), headerResponseData.cend());
                    auto i = statusLineResult.first;

                    response.status = std::move(statusLineResult.second);

                    for (;;)
                    {
                        const auto headerFieldResult = parseHeaderField(i, headerResponseData.cend());
                        i = headerFieldResult.first;

                        auto fieldName = std::move(headerFieldResult.second.first);

                        const auto toLower = [](const char c) noexcept {
                            return (c >= 'A' && c <= 'Z') ? c - ('A' - 'a') : c;
                        };

                        std::transform(fieldName.begin(), fieldName.end(), fieldName.begin(), toLower);

                        auto fieldValue = std::move(headerFieldResult.second.second);

                        if (fieldName == "transfer-encoding")
                        {
                            // RFC 7230, 3.3.1. Transfer-Encoding
                            if (fieldValue == "chunked")
                                chunkedResponse = true;
                            else
                                throw ResponseError{"Unsupported transfer encoding: " + fieldValue};
                        }
                        else if (fieldName == "content-length")
                        {
                            // RFC 7230, 3.3.2. Content-Length
                            contentLength = static_cast<std::size_t>(std::stoul(fieldValue));
                            contentLengthReceived = true;
                            response.body.reserve(contentLength);
                        }

                        response.headers.push_back(std::make_pair(std::move(fieldName), std::move(fieldValue)));

                        if (i == headerResponseData.cend())
                            break;
                    }

                    parsingBody = true;
                }

                if (parsingBody)
                {
                    // Content-Length must be ignored if Transfer-Encoding is received (RFC 7230, 3.2. Content-Length)
                    if (chunkedResponse)
                    {
                        // RFC 7230, 4.1. Chunked Transfer Coding
                        for (;;)
                        {
                            if (expectedChunkSize > 0)
                            {
                                const auto toWrite = (std::min)(expectedChunkSize, responseData.size());
                                response.body.insert(response.body.end(), responseData.begin(),
                                                     responseData.begin() + static_cast<std::ptrdiff_t>(toWrite));
                                responseData.erase(responseData.begin(),
                                                   responseData.begin() + static_cast<std::ptrdiff_t>(toWrite));
                                expectedChunkSize -= toWrite;

                                if (expectedChunkSize == 0) removeCrlfAfterChunk = true;
                                if (responseData.empty()) break;
                            }
                            else
                            {
                                if (removeCrlfAfterChunk)
                                {
                                    if (responseData.size() < 2) break;
                                    
                                    if (!std::equal(crlf.begin(), crlf.end(), responseData.begin()))
                                        throw ResponseError{"Invalid chunk"};

                                    removeCrlfAfterChunk = false;
                                    responseData.erase(responseData.begin(), responseData.begin() + 2);
                                }

                                const auto i = std::search(responseData.begin(), responseData.end(),
                                                           crlf.begin(), crlf.end());

                                if (i == responseData.end()) break;

                                const std::string line(responseData.begin(), i);
                                responseData.erase(responseData.begin(), i + 2);

                                expectedChunkSize = detail::hexToUint<std::size_t>(line.begin(), line.end());
                                if (expectedChunkSize == 0)
                                    return response;
                            }
                        }
                    }
                    else
                    {
                        response.body.insert(response.body.end(), responseData.begin(), responseData.end());
                        responseData.clear();

                        // got the whole content
                        if (contentLengthReceived && response.body.size() >= contentLength)
                            return response;
                    }
                }
            }

            return response;
        }

    private:
#if defined(_WIN32) || defined(__CYGWIN__)
        WinSock winSock;
#endif // defined(_WIN32) || defined(__CYGWIN__)
        InternetProtocol internetProtocol;
        Uri uri;
    };
}

#endif // HTTPREQUEST_HPP
