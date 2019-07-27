//
//  HTTPRequest
//

#include <iostream>
#include <fstream>
#include "HTTPRequest.hpp"

int main(int argc, const char * argv[])
{
    try
    {
        std::string url;
        std::string method = "GET";
        std::string arguments;
        std::string output;
        http::InternetProtocol protocol = http::InternetProtocol::V4;

        for (int i = 1; i < argc; ++i)
        {
            if (std::string(argv[i]) == "--help")
            {
                std::cout << "test --url <url> [--protocol <protocol>] [--method <method>] [--arguments <arguments>] [--output <output>]\n";
                return EXIT_SUCCESS;
            }
            else if (std::string(argv[i]) == "--url")
            {
                if (++i < argc) url = argv[i];
                else throw std::runtime_error("Missing argument for --url");
            }
            else if (std::string(argv[i]) == "--protocol")
            {
                if (++i < argc)
                {
                    if (std::string(argv[i]) == "ipv4")
                        protocol = http::InternetProtocol::V4;
                    else if (std::string(argv[i]) == "ipv6")
                        protocol = http::InternetProtocol::V6;
                    else
                        throw std::runtime_error("Invalid protocol");
                }
                else throw std::runtime_error("Missing argument for --protocol");
            }
            else if (std::string(argv[i]) == "--method")
            {
                if (++i < argc) method = argv[i];
                else throw std::runtime_error("Missing argument for --method");
            }
            else if (std::string(argv[i]) == "--arguments")
            {
                if (++i < argc) arguments = argv[i];
                else throw std::runtime_error("Missing argument for --arguments");
            }
            else if (std::string(argv[i]) == "--output")
            {
                if (++i < argc) output = argv[i];
                else throw std::runtime_error("Missing argument for --output");
            }
        }

        http::Request request(url, protocol);

        http::Response response = request.send(method, arguments, {
            "Content-Type: application/x-www-form-urlencoded",
            "User-Agent: runscope/0.1"
        });

        if (response.status == http::Response::STATUS_OK &&
            !output.empty())
        {
            std::ofstream outfile(output, std::ofstream::binary);
            outfile.write(reinterpret_cast<const char*>(response.body.data()),
                          static_cast<std::streamsize>(response.body.size()));
        }
        else
            std::cout << std::string(response.body.begin(), response.body.end()) << "\n";
    }
    catch (const std::exception& e)
    {
        std::cerr << "Request failed, error: " << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
