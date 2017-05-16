//
//  HTTPRequest
//

#include <iostream>
#include "HTTPRequest.h"

int main(int argc, const char * argv[])
{
    std::string url;
    std::string method;
    std::string arguments;

    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]) == "--url")
        {
            if (++i < argc) url = argv[i];
        }
        else if (std::string(argv[i]) == "--method")
        {
            if (++i < argc) method = argv[i];
        }
        else if (std::string(argv[i]) == "--arguments")
        {
            if (++i < argc) arguments = argv[i];
        }
    }

    http::Request request(url);

    http::Response response = request.send(method, arguments, {
        "Content-Type: application/x-www-form-urlencoded",
        "User-Agent: runscope/0.1"
    });

    if (response.succeeded)
    {
        std::cout << response.body.data() << std::endl;
    }
    else
    {
        std::cout << "Request failed" << std::endl;
    }

    return 0;
}
