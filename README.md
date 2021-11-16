# HTTPRequest

HTTPRequest is a single-header library for making HTTP requests. You can just include it in your project and use it. HTTPRequest was tested on macOS, Windows, Haiku, BSD, and GNU/Linux, but it should work on most of the Linux-based platforms. Supports IPv4 and IPv6.

## Usage

### Example of GET request
```cpp
#include "HTTPRequest.hpp"

try
{
    // you can pass http::InternetProtocol::V6 to Request to make an IPv6 request
    http::Request request{"http://test.com/test"};

    // send a get request
    const auto response = request.send("GET");
    std::cout << std::string{response.body.begin(), response.body.end()} << '\n'; // print the result
}
catch (const std::exception& e)
{
    std::cerr << "Request failed, error: " << e.what() << '\n';
}
```

### Example of POST request
```cpp
#include "HTTPRequest.hpp"

try
{
    http::Request request{"http://test.com/test"};
    // send a post request
    const auto response = request.send("POST", "foo=1&bar=baz", {
        "Content-Type: application/x-www-form-urlencoded"
    });
    std::cout << std::string{response.body.begin(), response.body.end()} << '\n'; // print the result
}
catch (const std::exception& e)
{
    std::cerr << "Request failed, error: " << e.what() << '\n';
}
```

### Example of POST request with form data
```cpp
#include "HTTPRequest.hpp"

try
{
    http::Request request{"http://test.com/test"};
    const string body = "foo=1&bar=baz";
    const auto response = request.send("POST", body, {
        "Content-Type: application/x-www-form-urlencoded"
    });
    std::cout << std::string{response.body.begin(), response.body.end()} << '\n'; // print the result
}
catch (const std::exception& e)
{
    std::cerr << "Request failed, error: " << e.what() << '\n';
}
```

### Example of POST request with a JSON body
```cpp
#include "HTTPRequest.hpp"

try
{
    http::Request request{"http://test.com/test"};
    const std::string body = "{\"foo\": 1, \"bar\": \"baz\"}";
    const auto response = request.send("POST", parameters, {
        "Content-Type: application/json"
    });
    std::cout << std::string{response.body.begin(), response.body.end()} << '\n'; // print the result
}
catch (const std::exception& e)
{
    std::cerr << "Request failed, error: " << e.what() << '\n';
}
```

To set a timeout for HTTP requests, pass `std::chrono::duration` as a last parameter to `send()`. A negative duration (default) passed to `send()` disables timeout.

## License

HTTPRequest is released to the Public Domain.
