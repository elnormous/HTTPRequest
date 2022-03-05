#include <cstddef>
#include "catch2/catch.hpp"
#include "HTTPRequest.hpp"

TEST_CASE("Encode status line", "[serialization]")
{
    const auto result = http::encodeRequestLine("GET", "/");
    REQUIRE(result == "GET / HTTP/1.1\r\n");
}

TEST_CASE("Encode header", "[serialization]")
{
    const auto result = http::encodeHeaders({
        {"a", "b"}
    });
    REQUIRE(result == "a: b\r\n");
}

TEST_CASE("Encode header without value", "[serialization]")
{
    const auto result = http::encodeHeaders({
        {"a", ""}
    });
    REQUIRE(result == "a: \r\n");
}

TEST_CASE("Encode headers", "[serialization]")
{
    const auto result = http::encodeHeaders({
        {"a", "b"},
        {"c", "d"}
    });
    REQUIRE(result == "a: b\r\nc: d\r\n");
}

TEST_CASE("Encode Base64", "[serialization]")
{
    const std::string str = "test:test";
    const auto result = http::encodeBase64(str.begin(), str.end());
    REQUIRE(result == "dGVzdDp0ZXN0");
}
