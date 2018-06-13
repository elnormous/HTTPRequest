//
//  HTTPRequest
//

#include <iostream>
#include <fstream>
#include "HTTPRequest.hpp"

int main(int argc, const char * argv[])
{
    std::string url;
    std::string method;
    std::string arguments;
    std::string output;

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
        else if (std::string(argv[i]) == "--output")
        {
            if (++i < argc) output = argv[i];
        }
    }

    try
    {
        http::Request request(url);

        http::Response response = request.send(method, arguments, {
            "Content-Type: application/x-www-form-urlencoded",
            "User-Agent: runscope/0.1"
        });

        if (!output.empty())
        {
            std::ofstream outfile(output, std::ofstream::binary);
            outfile.write(reinterpret_cast<const char*>(response.body.data()),
                          response.body.size());
        }
        else
            std::cout << response.body.data() << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Request failed, error: " << e.what() << std::endl;
    }

    return 0;
}
