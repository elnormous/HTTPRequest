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
    const auto result = http::encodeHeaderFields({
        {"a", "b"}
    });
    REQUIRE(result == "a: b\r\n");
}

TEST_CASE("Encode header without value", "[serialization]")
{
    const auto result = http::encodeHeaderFields({
        {"a", ""}
    });
    REQUIRE(result == "a: \r\n");
}

TEST_CASE("Encode headers", "[serialization]")
{
    const auto result = http::encodeHeaderFields({
        {"a", "b"},
        {"c", "d"}
    });
    REQUIRE(result == "a: b\r\nc: d\r\n");
}

TEST_CASE("Encode header with an empty name", "[serialization]")
{
    REQUIRE_THROWS_AS(http::encodeHeaderFields({
        {"", "b"}
    }), http::RequestError);
}

TEST_CASE("Encode header with a new-line in name", "[serialization]")
{
    REQUIRE_THROWS_AS(http::encodeHeaderFields({
        {"a\n", ""}
    }), http::RequestError);
}

TEST_CASE("Encode header with a new-line in value", "[serialization]")
{
    REQUIRE_THROWS_AS(http::encodeHeaderFields({
        {"a", "\n"}
    }), http::RequestError);
}

TEST_CASE("Encode Base64", "[serialization]")
{
    const std::string str = "test:test";
    const auto result = http::encodeBase64(str.begin(), str.end());
    REQUIRE(result == "dGVzdDp0ZXN0");
}

TEST_CASE("Encode HTML", "[serialization]")
{
    http::Uri uri;
    uri.scheme = "http";
    uri.path = "/";
    uri.host = "test.com";

    const auto result = http::detail::encodeHtml(uri, "GET", {}, {});
    const std::string check = "GET / HTTP/1.1\r\nHost: test.com\r\nContent-Length: 0\r\n\r\n";

    REQUIRE(check.size() == result.size());

    for (std::size_t i = 0; i < check.size(); ++i)
        REQUIRE(static_cast<uint8_t>(check[i]) == result[i]);
}

TEST_CASE("Encode HTML with data", "[serialization]")
{
    http::Uri uri;
    uri.scheme = "http";
    uri.path = "/";
    uri.host = "test.com";
    const std::vector<std::uint8_t> body = {'1'};

    const auto result = http::detail::encodeHtml(uri, "GET", body, {});
    const std::string check = "GET / HTTP/1.1\r\nHost: test.com\r\nContent-Length: 1\r\n\r\n1";

    REQUIRE(check.size() == result.size());

    for (std::size_t i = 0; i < check.size(); ++i)
        REQUIRE(static_cast<uint8_t>(check[i]) == result[i]);
}
