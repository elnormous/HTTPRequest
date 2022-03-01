//
//  HTTPRequest
//

#include <iostream>
#include <fstream>
#include "HTTPRequest.hpp"

int main(int argc, const char* argv[])
{
    try
    {
        std::string uri;
        std::string method = "GET";
        std::string arguments;
        std::string output;
        auto protocol = http::InternetProtocol::V4;

        for (int i = 1; i < argc; ++i)
        {
            const auto arg = std::string{argv[i]};

            if (arg == "--help")
            {
                std::cout << "example --url <url> [--protocol <protocol>] [--method <method>] [--arguments <arguments>] [--output <output>]\n";
                return EXIT_SUCCESS;
            }
            else if (arg == "--uri")
            {
                if (++i < argc) uri = argv[i];
                else throw std::runtime_error("Missing argument for --url");
            }
            else if (arg == "--protocol")
            {
                if (++i < argc)
                {
                    if (std::string{argv[i]} == "ipv4")
                        protocol = http::InternetProtocol::V4;
                    else if (std::string{argv[i]} == "ipv6")
                        protocol = http::InternetProtocol::V6;
                    else
                        throw std::runtime_error{"Invalid protocol"};
                }
                else throw std::runtime_error{"Missing argument for --protocol"};
            }
            else if (arg == "--method")
            {
                if (++i < argc) method = argv[i];
                else throw std::runtime_error{"Missing argument for --method"};
            }
            else if (arg == "--arguments")
            {
                if (++i < argc) arguments = argv[i];
                else throw std::runtime_error{"Missing argument for --arguments"};
            }
            else if (arg == "--output")
            {
                if (++i < argc) output = argv[i];
                else throw std::runtime_error{"Missing argument for --output"};
            }
            else
                throw std::runtime_error{"Invalid flag: " + arg};
        }

        http::Request request{uri, protocol};

        const auto response = request.send(method, arguments, {
            {"Content-Type", "application/x-www-form-urlencoded"},
            {"User-Agent", "runscope/0.1"},
            {"Accept", "*/*"}
        }, std::chrono::seconds(2));

        std::cout << response.status.reason << '\n';

        if (response.status.code == http::Status::Ok)
        {
            if (!output.empty())
            {
                std::ofstream outfile{output, std::ofstream::binary};
                outfile.write(reinterpret_cast<const char*>(response.body.data()),
                              static_cast<std::streamsize>(response.body.size()));
            }
            else
                std::cout << std::string{response.body.begin(), response.body.end()} << '\n';
        }
    }
    catch (const http::RequestError& e)
    {
        std::cerr << "Request error: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
    catch (const http::ResponseError& e)
    {
        std::cerr << "Response error: " << e.what() << '\n';
        return EXIT_FAILURE;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
