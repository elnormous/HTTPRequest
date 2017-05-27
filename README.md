# HTTPRequest

HTTPRequest is a single-header library for making HTTP requests. You can just include it in your project and use it.

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
