# HTTPRequest

HTTPRequest is a single-header library for making HTTP requests. You can just include it in your project and use it.
| :heavy_check_mark: Supports IPv4 and IPv6 |
| --- |

# HTTPRequest was tested on
- MacOS
- Windows
- GNU/Linux
- Android
- BSD
- Haiku

## Usage
<details>
  <summary>Example of GET request</summary>

  ```cpp
  #include <iostream>

  #include "HTTPRequest.hpp"
  
  int main(void)
  {
      try
      {
          // create a request
          // you can pass http::InternetProtocol::V6 to Request to make an IPv6 request
          http::Request request{"http://test.com/test"};

          // send a get request
          const auto response = request.send("GET");

          // print the result
          std::cout << std::string{response.body.begin(), response.body.end()} << '\n';
      }
      catch (const std::exception& e)
      {
          std::cerr << "Request failed, error: " << e.what() << '\n';
      }
  }
  ```
</details>
<details>
  <summary>Example of POST request</summary>

  ```cpp
  #include <iostream>

  #include "HTTPRequest.hpp"
  
  int main(void)
  {
      try
      {
          // create a request
          http::Request request{"http://test.com/test"};

          // send a post request
          const auto response = request.send("POST", "foo=1&bar=baz", {
              "Content-Type: application/x-www-form-urlencoded"
          });

          // print the result
          std::cout << std::string{response.body.begin(), response.body.end()} << '\n';
      }
      catch (const std::exception& e)
      {
          std::cerr << "Request failed, error: " << e.what() << '\n';
      }
  }
  ```
</details>
<details>
    <summary>Example of POST request by passing a map of parameters</summary>

  ```cpp
  #include <iostream>
  #include <map>

  #include "HTTPRequest.hpp"
  
  int main(void)
  {
      try
      {
          // pass parameters as a map
          std::map<std::string, std::string> parameters = {{"foo", "1"}, {"bar", "baz"}};

          // create a request
          http::Request request("http://test.com/test");

          // send a post request
          const auto response = request.send("POST", parameters, {
              "Content-Type: application/x-www-form-urlencoded"
          });

          // print the result
          std::cout << std::string{response.body.begin(), response.body.end()} << '\n';
      }
      catch (const std::exception& e)
      {
          std::cerr << "Request failed, error: " << e.what() << '\n';
      }
  }  
  ```
</details>

To set a timeout for HTTP requests, pass `std::chrono::duration` as a last parameter to `send()`. A negative duration (default) passed to `send()` disables timeout.

## License

HTTPRequest is released to the Public Domain.
