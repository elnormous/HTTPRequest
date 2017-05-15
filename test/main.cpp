//
//  main.cpp
//  test
//
//  Created by Elviss Strazdins on 15/05/2017.
//  Copyright © 2017 Elviss Strazdins. All rights reserved.
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

    HTTPRequest request(url);
    if (!request.send())
    {
        std::cerr << "Failed to send request" << std::endl;
    }

    std::cout << "Hello, World!\n";
    return 0;
}
