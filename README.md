# HTTPRequest

HTTPRequest is a class for making HTTP requests

Usage:
```
http::Request request("http://test.com/post");
request.send("POST", "foo=1&bar=baz"), {
    "Content-Type: application/x-www-form-urlencoded"
});
std::cout << response.body.data() << std::endl; // print the result
```

## License

HTTPRequest codebase is licensed under the BSD license. Please refer to the LICENSE file for detailed information.
