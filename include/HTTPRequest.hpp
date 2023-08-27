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
#include <utility>
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
        using logic_error::logic_error;
        using logic_error::operator=;
    };

    class ResponseError final: public std::runtime_error
    {
    public:
        using runtime_error::runtime_error;
        using runtime_error::operator=;
    };

    enum class InternetProtocol: std::uint8_t
    {
        v4,
        v6
    };

    struct Uri final
    {
        std::string scheme;
        std::string user;
        std::string password;
        std::string host;
        std::string port;
        std::string path;
        std::string query;
        std::string fragment;
    };

    struct Version final
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

        Version version;
        std::uint16_t code;
        std::string reason;
    };

    using HeaderField = std::pair<std::string, std::string>;
    using HeaderFields = std::vector<HeaderField>;

    struct Response final
    {
        Status status;
        HeaderFields headerFields;
        std::vector<std::uint8_t> body;
    };

    inline namespace detail
    {
#if defined(_WIN32) || defined(__CYGWIN__)
        namespace winsock
        {
            class ErrorCategory final: public std::error_category
            {
            public:
                const char* name() const noexcept override
                {
                    return "Windows Sockets API";
                }

                std::string message(const int condition) const override
                {
                    switch (condition)
                    {
                        case WSA_INVALID_HANDLE: return "Specified event object handle is invalid";
                        case WSA_NOT_ENOUGH_MEMORY: return "Insufficient memory available";
                        case WSA_INVALID_PARAMETER: return "One or more parameters are invalid";
                        case WSA_OPERATION_ABORTED: return "Overlapped operation aborted";
                        case WSA_IO_INCOMPLETE: return "Overlapped I/O event object not in signaled state";
                        case WSA_IO_PENDING: return "Overlapped operations will complete later";
                        case WSAEINTR: return "Interrupted function call";
                        case WSAEBADF: return "File handle is not valid";
                        case WSAEACCES: return "Permission denied";
                        case WSAEFAULT: return "Bad address";
                        case WSAEINVAL: return "Invalid argument";
                        case WSAEMFILE: return "Too many open files";
                        case WSAEWOULDBLOCK: return "Resource temporarily unavailable";
                        case WSAEINPROGRESS: return "Operation now in progress";
                        case WSAEALREADY: return "Operation already in progress";
                        case WSAENOTSOCK: return "Socket operation on nonsocket";
                        case WSAEDESTADDRREQ: return "Destination address required";
                        case WSAEMSGSIZE: return "Message too long";
                        case WSAEPROTOTYPE: return "Protocol wrong type for socket";
                        case WSAENOPROTOOPT: return "Bad protocol option";
                        case WSAEPROTONOSUPPORT: return "Protocol not supported";
                        case WSAESOCKTNOSUPPORT: return "Socket type not supported";
                        case WSAEOPNOTSUPP: return "Operation not supported";
                        case WSAEPFNOSUPPORT: return "Protocol family not supported";
                        case WSAEAFNOSUPPORT: return "Address family not supported by protocol family";
                        case WSAEADDRINUSE: return "Address already in use";
                        case WSAEADDRNOTAVAIL: return "Cannot assign requested address";
                        case WSAENETDOWN: return "Network is down";
                        case WSAENETUNREACH: return "Network is unreachable";
                        case WSAENETRESET: return "Network dropped connection on reset";
                        case WSAECONNABORTED: return "Software caused connection abort";
                        case WSAECONNRESET: return "Connection reset by peer";
                        case WSAENOBUFS: return "No buffer space available";
                        case WSAEISCONN: return "Socket is already connected";
                        case WSAENOTCONN: return "Socket is not connected";
                        case WSAESHUTDOWN: return "Cannot send after socket shutdown";
                        case WSAETOOMANYREFS: return "Too many references";
                        case WSAETIMEDOUT: return "Connection timed out";
                        case WSAECONNREFUSED: return "Connection refused";
                        case WSAELOOP: return "Cannot translate name";
                        case WSAENAMETOOLONG: return "Name too long";
                        case WSAEHOSTDOWN: return "Host is down";
                        case WSAEHOSTUNREACH: return "No route to host";
                        case WSAENOTEMPTY: return "Directory not empty";
                        case WSAEPROCLIM: return "Too many processes";
                        case WSAEUSERS: return "User quota exceeded";
                        case WSAEDQUOT: return "Disk quota exceeded";
                        case WSAESTALE: return "Stale file handle reference";
                        case WSAEREMOTE: return "Item is remote";
                        case WSASYSNOTREADY: return "Network subsystem is unavailable";
                        case WSAVERNOTSUPPORTED: return "Winsock.dll version out of range";
                        case WSANOTINITIALISED: return "Successful WSAStartup not yet performed";
                        case WSAEDISCON: return "Graceful shutdown in progress";
                        case WSAENOMORE: return "No more results";
                        case WSAECANCELLED: return "Call has been canceled";
                        case WSAEINVALIDPROCTABLE: return "Procedure call table is invalid";
                        case WSAEINVALIDPROVIDER: return "Service provider is invalid";
                        case WSAEPROVIDERFAILEDINIT: return "Service provider failed to initialize";
                        case WSASYSCALLFAILURE: return "System call failure";
                        case WSASERVICE_NOT_FOUND: return "Service not found";
                        case WSATYPE_NOT_FOUND: return "Class type not found";
                        case WSA_E_NO_MORE: return "No more results";
                        case WSA_E_CANCELLED: return "Call was canceled";
                        case WSAEREFUSED: return "Database query was refused";
                        case WSAHOST_NOT_FOUND: return "Host not found";
                        case WSATRY_AGAIN: return "Nonauthoritative host not found";
                        case WSANO_RECOVERY: return "This is a nonrecoverable error";
                        case WSANO_DATA: return "Valid name, no data record of requested type";
                        case WSA_QOS_RECEIVERS: return "QoS receivers";
                        case WSA_QOS_SENDERS: return "QoS senders";
                        case WSA_QOS_NO_SENDERS: return "No QoS senders";
                        case WSA_QOS_NO_RECEIVERS: return "QoS no receivers";
                        case WSA_QOS_REQUEST_CONFIRMED: return "QoS request confirmed";
                        case WSA_QOS_ADMISSION_FAILURE: return "QoS admission error";
                        case WSA_QOS_POLICY_FAILURE: return "QoS policy failure";
                        case WSA_QOS_BAD_STYLE: return "QoS bad style";
                        case WSA_QOS_BAD_OBJECT: return "QoS bad object";
                        case WSA_QOS_TRAFFIC_CTRL_ERROR: return "QoS traffic control error";
                        case WSA_QOS_GENERIC_ERROR: return "QoS generic error";
                        case WSA_QOS_ESERVICETYPE: return "QoS service type error";
                        case WSA_QOS_EFLOWSPEC: return "QoS flowspec error";
                        case WSA_QOS_EPROVSPECBUF: return "Invalid QoS provider buffer";
                        case WSA_QOS_EFILTERSTYLE: return "Invalid QoS filter style";
                        case WSA_QOS_EFILTERTYPE: return "Invalid QoS filter type";
                        case WSA_QOS_EFILTERCOUNT: return "Incorrect QoS filter count";
                        case WSA_QOS_EOBJLENGTH: return "Invalid QoS object length";
                        case WSA_QOS_EFLOWCOUNT: return "Incorrect QoS flow count";
                        case WSA_QOS_EUNKOWNPSOBJ: return "Unrecognized QoS object";
                        case WSA_QOS_EPOLICYOBJ: return "Invalid QoS policy object";
                        case WSA_QOS_EFLOWDESC: return "Invalid QoS flow descriptor";
                        case WSA_QOS_EPSFLOWSPEC: return "Invalid QoS provider-specific flowspec";
                        case WSA_QOS_EPSFILTERSPEC: return "Invalid QoS provider-specific filterspec";
                        case WSA_QOS_ESDMODEOBJ: return "Invalid QoS shape discard mode object";
                        case WSA_QOS_ESHAPERATEOBJ: return "Invalid QoS shaping rate object";
                        case WSA_QOS_RESERVED_PETYPE: return "Reserved policy QoS element type";
                        default: return "Unknown error (" + std::to_string(condition) + ")";
                    }
                }
            };

            inline const ErrorCategory errorCategory;

            class Api final
            {
            public:
                Api()
                {
                    WSADATA wsaData;
                    const auto error = WSAStartup(MAKEWORD(2, 2), &wsaData);
                    if (error != 0)
                        throw std::system_error{error, errorCategory, "WSAStartup failed"};

                    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)
                    {
                        WSACleanup();
                        throw std::runtime_error{"Invalid WinSock version"};
                    }

                    started = true;
                }

                ~Api()
                {
                    if (started) WSACleanup();
                }

                Api(Api&& other) noexcept:
                    started{std::exchange(other.started, false)}
                {
                }

                Api& operator=(Api&& other) noexcept
                {
                    if (&other == this) return *this;
                    if (started) WSACleanup();
                    started = std::exchange(other.started, false);
                    return *this;
                }

            private:
                bool started = false;
            };
        }
#endif // defined(_WIN32) || defined(__CYGWIN__)

        constexpr int getAddressFamily(const InternetProtocol internetProtocol)
        {
            return (internetProtocol == InternetProtocol::v4) ? AF_INET :
                (internetProtocol == InternetProtocol::v6) ? AF_INET6 :
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
#if defined(_WIN32) || defined(__CYGWIN__)
                    throw std::system_error{WSAGetLastError(), winsock::errorCategory, "Failed to create socket"};
#else
                    throw std::system_error{errno, std::system_category(), "Failed to create socket"};
#endif // defined(_WIN32) || defined(__CYGWIN__)

#if defined(_WIN32) || defined(__CYGWIN__)
                ULONG mode = 1;
                if (ioctlsocket(endpoint, FIONBIO, &mode) == SOCKET_ERROR)
                {
                    close();
                    throw std::system_error{WSAGetLastError(), winsock::errorCategory, "Failed to get socket flags"};
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
                endpoint{std::exchange(other.endpoint, invalid)}
            {
            }

            Socket& operator=(Socket&& other) noexcept
            {
                if (&other == this) return *this;
                if (endpoint != invalid) close();
                endpoint = std::exchange(other.endpoint, invalid);
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
                        if (getsockopt(endpoint, SOL_SOCKET, SO_ERROR, socketErrorPointer, &optionLength) == SOCKET_ERROR)
                            throw std::system_error{WSAGetLastError(), winsock::errorCategory, "Failed to get socket option"};

                        int socketError;
                        std::memcpy(&socketError, socketErrorPointer, sizeof(socketErrorPointer));

                        if (socketError != 0)
                            throw std::system_error{socketError, winsock::errorCategory, "Failed to connect"};
                    }
                    else
                        throw std::system_error{WSAGetLastError(), winsock::errorCategory, "Failed to connect"};
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

                while (result == SOCKET_ERROR && WSAGetLastError() == WSAEINTR)
                    result = ::send(endpoint, reinterpret_cast<const char*>(buffer),
                                    static_cast<int>(length), 0);

                if (result == SOCKET_ERROR)
                    throw std::system_error{WSAGetLastError(), winsock::errorCategory, "Failed to send data"};
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

                while (result == SOCKET_ERROR && WSAGetLastError() == WSAEINTR)
                    result = ::recv(endpoint, reinterpret_cast<char*>(buffer),
                                    static_cast<int>(length), 0);

                if (result == SOCKET_ERROR)
                    throw std::system_error{WSAGetLastError(), winsock::errorCategory, "Failed to read data"};
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

                while (count == SOCKET_ERROR && WSAGetLastError() == WSAEINTR)
                    count = ::select(0,
                                     (type == SelectType::read) ? &descriptorSet : nullptr,
                                     (type == SelectType::write) ? &descriptorSet : nullptr,
                                     nullptr,
                                     (timeout >= 0) ? &selectTimeout : nullptr);

                if (count == SOCKET_ERROR)
                    throw std::system_error{WSAGetLastError(), winsock::errorCategory, "Failed to select socket"};
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

        inline char toLower(const char c) noexcept
        {
            return (c >= 'A' && c <= 'Z') ? c - ('A' - 'a') : c;
        }

        template <class T>
        T toLower(const T& s)
        {
            T result = s;
            for (auto& c : result) c = toLower(c);
            return result;
        }

        // RFC 7230, 3.2.3. WhiteSpace
        template <typename C>
        constexpr bool isWhiteSpaceChar(const C c) noexcept
        {
            return c == 0x20 || c == 0x09; // space or tab
        };

        // RFC 5234, Appendix B.1. Core Rules
        template <typename C>
        constexpr bool isDigitChar(const C c) noexcept
        {
            return c >= 0x30 && c <= 0x39; // 0 - 9
        }

        // RFC 5234, Appendix B.1. Core Rules
        template <typename C>
        constexpr bool isAlphaChar(const C c) noexcept
        {
            return
                (c >= 0x61 && c <= 0x7A) || // a - z
                (c >= 0x41 && c <= 0x5A); // A - Z
        }

        // RFC 7230, 3.2.6. Field Value Components
        template <typename C>
        constexpr bool isTokenChar(const C c) noexcept
        {
            return c == 0x21 || // !
                c == 0x23 || // #
                c == 0x24 || // $
                c == 0x25 || // %
                c == 0x26 || // &
                c == 0x27 || // '
                c == 0x2A || // *
                c == 0x2B || // +
                c == 0x2D || // -
                c == 0x2E || // .
                c == 0x5E || // ^
                c == 0x5F || // _
                c == 0x60 || // `
                c == 0x7C || // |
                c == 0x7E || // ~
                isDigitChar(c) ||
                isAlphaChar(c);
        };

        // RFC 5234, Appendix B.1. Core Rules
        template <typename C>
        constexpr bool isVisibleChar(const C c) noexcept
        {
            return c >= 0x21 && c <= 0x7E;
        }

        // RFC 7230, Appendix B. Collected ABNF
        template <typename C>
        constexpr bool isObsoleteTextChar(const C c) noexcept
        {
            return static_cast<unsigned char>(c) >= 0x80 &&
                static_cast<unsigned char>(c) <= 0xFF;
        }

        template <class Iterator>
        Iterator skipWhiteSpaces(const Iterator begin, const Iterator end)
        {
            auto i = begin;
            for (i = begin; i != end; ++i)
                if (!isWhiteSpaceChar(*i))
                    break;

            return i;
        }

        // RFC 5234, Appendix B.1. Core Rules
        template <typename T, typename C, typename std::enable_if<std::is_unsigned<T>::value>::type* = nullptr>
        constexpr T digitToUint(const C c)
        {
            // DIGIT
            return (c >= 0x30 && c <= 0x39) ? static_cast<T>(c - 0x30) : // 0 - 9
                throw ResponseError{"Invalid digit"};
        }

        // RFC 5234, Appendix B.1. Core Rules
        template <typename T, typename C, typename std::enable_if<std::is_unsigned<T>::value>::type* = nullptr>
        constexpr T hexDigitToUint(const C c)
        {
            // HEXDIG
            return (c >= 0x30 && c <= 0x39) ? static_cast<T>(c - 0x30) : // 0 - 9
                (c >= 0x41 && c <= 0x46) ? static_cast<T>(c - 0x41) + T(10) : // A - Z
                (c >= 0x61 && c <= 0x66) ? static_cast<T>(c - 0x61) + T(10) : // a - z, some services send lower-case hex digits
                throw ResponseError{"Invalid hex digit"};
        }

        // RFC 3986, 3. Syntax Components
        template <class Iterator>
        Uri parseUri(const Iterator begin, const Iterator end)
        {
            Uri result;

            // RFC 3986, 3.1. Scheme
            auto i = begin;
            if (i == end || !isAlphaChar(*begin))
                throw RequestError{"Invalid scheme"};

            result.scheme.push_back(*i++);

            for (; i != end && (isAlphaChar(*i) || isDigitChar(*i) || *i == '+' || *i == '-' || *i == '.'); ++i)
                result.scheme.push_back(*i);

            if (i == end || *i++ != ':')
                throw RequestError{"Invalid scheme"};
            if (i == end || *i++ != '/')
                throw RequestError{"Invalid scheme"};
            if (i == end || *i++ != '/')
                throw RequestError{"Invalid scheme"};

            // RFC 3986, 3.2. Authority
            std::string authority = std::string(i, end);

            // RFC 3986, 3.5. Fragment
            const auto fragmentPosition = authority.find('#');
            if (fragmentPosition != std::string::npos)
            {
                result.fragment = authority.substr(fragmentPosition + 1);
                authority.resize(fragmentPosition); // remove the fragment part
            }

            // RFC 3986, 3.4. Query
            const auto queryPosition = authority.find('?');
            if (queryPosition != std::string::npos)
            {
                result.query = authority.substr(queryPosition + 1);
                authority.resize(queryPosition); // remove the query part
            }

            // RFC 3986, 3.3. Path
            const auto pathPosition = authority.find('/');
            if (pathPosition != std::string::npos)
            {
                // RFC 3986, 3.3. Path
                result.path = authority.substr(pathPosition);
                authority.resize(pathPosition);
            }
            else
                result.path = "/";

            // RFC 3986, 3.2.1. User Information
            std::string userinfo;
            const auto hostPosition = authority.find('@');
            if (hostPosition != std::string::npos)
            {
                userinfo = authority.substr(0, hostPosition);

                const auto passwordPosition = userinfo.find(':');
                if (passwordPosition != std::string::npos)
                {
                    result.user = userinfo.substr(0, passwordPosition);
                    result.password = userinfo.substr(passwordPosition + 1);
                }
                else
                    result.user = userinfo;

                result.host = authority.substr(hostPosition + 1);
            }
            else
                result.host = authority;

            // RFC 3986, 3.2.2. Host
            const auto portPosition = result.host.find(':');
            if (portPosition != std::string::npos)
            {
                // RFC 3986, 3.2.3. Port
                result.port = result.host.substr(portPosition + 1);
                result.host.resize(portPosition);
            }

            return result;
        }

        // RFC 7230, 2.6. Protocol Versioning
        template <class Iterator>
        std::pair<Iterator, Version> parseVersion(const Iterator begin, const Iterator end)
        {
            auto i = begin;

            if (i == end || *i++ != 'H')
                throw ResponseError{"Invalid HTTP version"};
            if (i == end || *i++ != 'T')
                throw ResponseError{"Invalid HTTP version"};
            if (i == end || *i++ != 'T')
                throw ResponseError{"Invalid HTTP version"};
            if (i == end || *i++ != 'P')
                throw ResponseError{"Invalid HTTP version"};
            if (i == end || *i++ != '/')
                throw ResponseError{"Invalid HTTP version"};

            if (i == end)
                throw ResponseError{"Invalid HTTP version"};

            const auto majorVersion = digitToUint<std::uint16_t>(*i++);

            if (i == end || *i++ != '.')
                throw ResponseError{"Invalid HTTP version"};

            if (i == end)
                throw ResponseError{"Invalid HTTP version"};

            const auto minorVersion = digitToUint<std::uint16_t>(*i++);

            return {i, Version{majorVersion, minorVersion}};
        }

        // RFC 7230, 3.1.2. Status Line
        template <class Iterator>
        std::pair<Iterator, std::uint16_t> parseStatusCode(const Iterator begin, const Iterator end)
        {
            std::uint16_t result = 0;

            auto i = begin;
            while (i != end && isDigitChar(*i))
                result = static_cast<std::uint16_t>(result * 10U) + digitToUint<std::uint16_t>(*i++);

            if (std::distance(begin, i) != 3)
                throw ResponseError{"Invalid status code"};

            return {i, result};
        }

        // RFC 7230, 3.1.2. Status Line
        template <class Iterator>
        std::pair<Iterator, std::string> parseReasonPhrase(const Iterator begin, const Iterator end)
        {
            std::string result;

            auto i = begin;
            for (; i != end && (isWhiteSpaceChar(*i) || isVisibleChar(*i) || isObsoleteTextChar(*i)); ++i)
                result.push_back(static_cast<char>(*i));

            return {i, std::move(result)};
        }

        // RFC 7230, 3.2.6. Field Value Components
        template <class Iterator>
        std::pair<Iterator, std::string> parseToken(const Iterator begin, const Iterator end)
        {
            std::string result;

            auto i = begin;
            for (; i != end && isTokenChar(*i); ++i)
                result.push_back(static_cast<char>(*i));

            if (result.empty())
                throw ResponseError{"Invalid token"};

            return {i, std::move(result)};
        }

        // RFC 7230, 3.2. Header Fields
        template <class Iterator>
        std::pair<Iterator, std::string> parseFieldValue(const Iterator begin, const Iterator end)
        {
            std::string result;

            auto i = begin;
            for (; i != end && (isWhiteSpaceChar(*i) || isVisibleChar(*i) || isObsoleteTextChar(*i)); ++i)
                result.push_back(static_cast<char>(*i));

            // trim white spaces
            result.erase(std::find_if(result.rbegin(), result.rend(), [](const char c) noexcept {
                return !isWhiteSpaceChar(c);
            }).base(), result.end());

            return {i, std::move(result)};
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

                if (obsoleteFoldIterator == end || !isWhiteSpaceChar(*obsoleteFoldIterator++))
                    break;

                result.push_back(' ');
                i = obsoleteFoldIterator;
            }

            return {i, std::move(result)};
        }

        // RFC 7230, 3.2. Header Fields
        template <class Iterator>
        std::pair<Iterator, HeaderField> parseHeaderField(const Iterator begin, const Iterator end)
        {
            auto tokenResult = parseToken(begin, end);
            auto i = tokenResult.first;
            auto fieldName = toLower(tokenResult.second);

            if (i == end || *i++ != ':')
                throw ResponseError{"Invalid header"};

            i = skipWhiteSpaces(i, end);

            auto valueResult = parseFieldContent(i, end);
            i = valueResult.first;
            auto fieldValue = std::move(valueResult.second);

            if (i == end || *i++ != '\r')
                throw ResponseError{"Invalid header"};

            if (i == end || *i++ != '\n')
                throw ResponseError{"Invalid header"};

            return {i, {std::move(fieldName), std::move(fieldValue)}};
        }

        // RFC 7230, 3.1.2. Status Line
        template <class Iterator>
        std::pair<Iterator, Status> parseStatusLine(const Iterator begin, const Iterator end)
        {
            const auto versionResult = parseVersion(begin, end);
            auto i = versionResult.first;

            if (i == end || *i++ != ' ')
                throw ResponseError{"Invalid status line"};

            const auto statusCodeResult = parseStatusCode(i, end);
            i = statusCodeResult.first;

            if (i == end || *i++ != ' ')
                throw ResponseError{"Invalid status line"};

            auto reasonPhraseResult = parseReasonPhrase(i, end);
            i = reasonPhraseResult.first;

            if (i == end || *i++ != '\r')
                throw ResponseError{"Invalid status line"};

            if (i == end || *i++ != '\n')
                throw ResponseError{"Invalid status line"};

            return {i, Status{
                versionResult.second,
                statusCodeResult.second,
                std::move(reasonPhraseResult.second)
            }};
        }

        // RFC 7230, 4.1. Chunked Transfer Coding
        template <typename T, class Iterator, typename std::enable_if<std::is_unsigned<T>::value>::type* = nullptr>
        T stringToUint(const Iterator begin, const Iterator end)
        {
            T result = 0;
            for (auto i = begin; i != end; ++i)
                result = T(10U) * result + digitToUint<T>(*i);

            return result;
        }

        template <typename T, class Iterator, typename std::enable_if<std::is_unsigned<T>::value>::type* = nullptr>
        T hexStringToUint(const Iterator begin, const Iterator end)
        {
            T result = 0;
            for (auto i = begin; i != end; ++i)
                result = T(16U) * result + hexDigitToUint<T>(*i);

            return result;
        }

        // RFC 7230, 3.1.1. Request Line
        inline std::string encodeRequestLine(const std::string& method, const std::string& target)
        {
            return method + " " + target + " HTTP/1.1\r\n";
        }

        // RFC 7230, 3.2. Header Fields
        inline std::string encodeHeaderFields(const HeaderFields& headerFields)
        {
            std::string result;
            for (const auto& headerField : headerFields)
            {
                if (headerField.first.empty())
                    throw RequestError{"Invalid header field name"};

                for (const auto c : headerField.first)
                    if (!isTokenChar(c))
                        throw RequestError{"Invalid header field name"};

                for (const auto c : headerField.second)
                    if (!isWhiteSpaceChar(c) && !isVisibleChar(c) && !isObsoleteTextChar(c))
                        throw RequestError{"Invalid header field value"};

                result += headerField.first + ": " + headerField.second + "\r\n";
            }

            return result;
        }

        // RFC 4648, 4. Base 64 Encoding
        template <class Iterator>
        std::string encodeBase64(const Iterator begin, const Iterator end)
        {
            constexpr std::array<char, 64> chars{
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

        inline std::vector<std::uint8_t> encodeHtml(const Uri& uri,
                                                    const std::string& method,
                                                    const std::vector<uint8_t>& body,
                                                    HeaderFields headerFields)
        {
            if (uri.scheme != "http")
                throw RequestError{"Only HTTP scheme is supported"};

            // RFC 7230, 5.3. Request Target
            const std::string requestTarget = uri.path + (uri.query.empty() ? ""  : '?' + uri.query);

            // RFC 7230, 5.4. Host
            headerFields.push_back({"Host", uri.host});

            // RFC 7230, 3.3.2. Content-Length
            headerFields.push_back({"Content-Length", std::to_string(body.size())});

            // RFC 7617, 2. The 'Basic' Authentication Scheme
            if (!uri.user.empty() || !uri.password.empty())
            {
                std::string userinfo = uri.user + ':' + uri.password;
                headerFields.push_back({"Authorization", "Basic " + encodeBase64(userinfo.begin(), userinfo.end())});
            }

            const auto headerData = encodeRequestLine(method, requestTarget) +
                encodeHeaderFields(headerFields) +
                "\r\n";

            std::vector<uint8_t> result(headerData.begin(), headerData.end());
            result.insert(result.end(), body.begin(), body.end());

            return result;
        }
    }

    class Request final
    {
    public:
        explicit Request(const std::string& uriString,
                         const InternetProtocol protocol = InternetProtocol::v4):
            internetProtocol{protocol},
            uri{parseUri(uriString.begin(), uriString.end())}
        {
        }

        Response send(const std::string& method = "GET",
                      const std::string& body = "",
                      const HeaderFields& headerFields = {},
                      const std::chrono::milliseconds timeout = std::chrono::milliseconds{-1})
        {
            return send(method,
                        std::vector<uint8_t>(body.begin(), body.end()),
                        headerFields,
                        timeout);
        }

        Response send(const std::string& method,
                      const std::vector<uint8_t>& body,
                      const HeaderFields& headerFields = {},
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
#if defined(_WIN32) || defined(__CYGWIN__)
                throw std::system_error{WSAGetLastError(), winsock::errorCategory, "Failed to get address info of " + uri.host};
#else
                throw std::system_error{errno, std::system_category(), "Failed to get address info of " + uri.host};
#endif // defined(_WIN32) || defined(__CYGWIN__)

            const std::unique_ptr<addrinfo, decltype(&freeaddrinfo)> addressInfo{info, freeaddrinfo};

            const auto requestData = encodeHtml(uri, method, body, headerFields);

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
                    const auto endIterator = std::search(responseData.cbegin(), responseData.cend(),
                                                         headerEnd.cbegin(), headerEnd.cend());
                    if (endIterator == responseData.cend()) break; // two consecutive CRLFs not found

                    const auto headerBeginIterator = responseData.cbegin();
                    const auto headerEndIterator = endIterator + 2;

                    auto statusLineResult = parseStatusLine(headerBeginIterator, headerEndIterator);
                    auto i = statusLineResult.first;

                    response.status = std::move(statusLineResult.second);

                    for (;;)
                    {
                        auto headerFieldResult = parseHeaderField(i, headerEndIterator);
                        i = headerFieldResult.first;

                        auto fieldName = std::move(headerFieldResult.second.first);
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
                            contentLength = stringToUint<std::size_t>(fieldValue.cbegin(), fieldValue.cend());
                            contentLengthReceived = true;
                            response.body.reserve(contentLength);
                        }

                        response.headerFields.push_back({std::move(fieldName), std::move(fieldValue)});

                        if (i == headerEndIterator)
                            break;
                    }

                    responseData.erase(responseData.cbegin(), headerEndIterator + 2);
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

                                expectedChunkSize = detail::hexStringToUint<std::size_t>(responseData.begin(), i);
                                responseData.erase(responseData.begin(), i + 2);

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
        winsock::Api winSock;
#endif // defined(_WIN32) || defined(__CYGWIN__)
        InternetProtocol internetProtocol;
        Uri uri;
    };
}

#endif // HTTPREQUEST_HPP
