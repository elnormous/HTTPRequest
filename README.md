# HTTPRequest

HTTPRequest is a single-header library for making HTTP requests. You can just include it in your project and use it. HTTPRequest was tested on macOS, Windows and Linux, but it should work on most of the Linux-based platforms. Supports IPv4 and IPv6.

## Usage
Example of GET request
```cpp
#include "HTTPRequest.hpp"

try
{
    // you can pass http::InternetProtocol::V6 to Request to make an IPv6 request
    http::Request request("http://test.com/test");

    // send a get request
    const http::Response response = request.send("GET");
    std::cout << std::string(response.body.begin(), response.body.end()) << '\n'; // print the result
}
catch (const std::exception& e)
{
    std::cerr << "Request failed, error: " << e.what() << '\n';
}
```

Example of POST request
```cpp
#include "HTTPRequest.hpp"

try
{
    http::Request request("http://test.com/test");
    // send a post request
    const http::Response response = request.send("POST", "foo=1&bar=baz", {
        "Content-Type: application/x-www-form-urlencoded"
    });
    std::cout << std::string(response.body.begin(), response.body.end()) << '\n'; // print the result
}
catch (const std::exception& e)
{
    std::cerr << "Request failed, error: " << e.what() << '\n';
}
```

Example of POST request by passing a map of parameters
```cpp
#include "HTTPRequest.hpp"

try
{
    http::Request request("http://test.com/test");
    // pass parameters as a map
    std::map<std::string, std::string> parameters = {{"foo", "1"}, {"bar", "baz"}};
    const http::Response response = request.send("POST", parameters, {
        "Content-Type: application/x-www-form-urlencoded"
    });
    std::cout << std::string(response.body.begin(), response.body.end()) << '\n'; // print the result
}
catch (const std::exception& e)
{
    std::cerr << "Request failed, error: " << e.what() << '\n';
}
```

## License

HTTPRequest is released to the Public Domain.
