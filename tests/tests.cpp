#include <cstddef>
#include "catch2/catch.hpp"
#include "HTTPRequest.hpp"

TEST_CASE("Whitespace", "[parsing]")
{
    REQUIRE(http::detail::isWhitespaceChar(' '));
    REQUIRE(http::detail::isWhitespaceChar('\t'));
}

TEST_CASE("Digit", "[parsing]")
{
    for (int c = 0; c < 256; ++c)
        REQUIRE(http::detail::isDigitChar(static_cast<char>(c)) == (c >= '0' && c <= '9'));
}

TEST_CASE("Alpha", "[parsing]")
{
    for (int c = 0; c < 256; ++c)
        REQUIRE(http::detail::isAlphaChar(static_cast<char>(c)) == ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')));
}

TEST_CASE("Token char", "[parsing]")
{
    for (int c = 0; c < 256; ++c)
        REQUIRE(http::detail::isTokenChar(static_cast<char>(c)) ==
                (c >= 0x21 && c <= 0x7E &&
                 c != '"' && c != '(' && c != ')' && c != ',' && c != '/' &&
                 c != ':' && c != ';' && c != '<' && c != '=' && c != '>' &&
                 c != '?' && c != '@' && c != '[' && c != '\\' && c != ']' &&
                 c != '{' && c != '}'));
}

TEST_CASE("Visible char", "[parsing]")
{
    for (int c = 0; c < 256; ++c)
        REQUIRE(http::detail::isVisibleChar(static_cast<char>(c)) == (c >= 0x21 && c <= 0x7E));
}

TEST_CASE("OBS text char", "[parsing]")
{
    for (int c = 0; c < 256; ++c)
        REQUIRE(http::detail::isObsoleteTextChar(static_cast<char>(c)) == (c >= 0x80 && c <= 0xFF));
}

TEST_CASE("Skip empty whitespaces", "[parsing]")
{
    std::string str = "";
    auto i = http::detail::skipWhitespaces(str.begin(), str.end());
    REQUIRE(i == str.begin());
    REQUIRE(i == str.end());
}

TEST_CASE("Skip one whitespace", "[parsing]")
{
    std::string str = " ";
    auto i = http::detail::skipWhitespaces(str.begin(), str.end());
    REQUIRE(i == str.end());
}

TEST_CASE("Skip one whitespace at the beggining", "[parsing]")
{
    std::string str = " a";
    auto i = http::detail::skipWhitespaces(str.begin(), str.end());
    REQUIRE(i == str.begin() + 1);
}

TEST_CASE("Don't skip whitespaces", "[parsing]")
{
    std::string str = "a ";
    auto i = http::detail::skipWhitespaces(str.begin(), str.end());
    REQUIRE(i == str.begin());
}

TEST_CASE("Parse token", "[parsing]")
{
    std::string str = "token";
    auto result = http::detail::parseToken(str.begin(), str.end());
    REQUIRE(result.first == str.end());
    REQUIRE(result.second == "token");
}

TEST_CASE("Parse HTTP version", "[parsing]")
{
    std::string str = "HTTP/1.1";
    auto result = http::detail::parseHttpVersion(str.begin(), str.end());
    REQUIRE(result.first == str.end());
    REQUIRE(result.second.major == 1U);
    REQUIRE(result.second.minor == 1U);
}

TEST_CASE("Invalid HTTP in version", "[parsing]")
{
    std::string str = "TTP/1.1";
    REQUIRE_THROWS_AS(http::detail::parseHttpVersion(str.begin(), str.end()), http::ResponseError);
}

TEST_CASE("No slash in HTTP version", "[parsing]")
{
    std::string str = "HTTP1.1";
    REQUIRE_THROWS_AS(http::detail::parseHttpVersion(str.begin(), str.end()), http::ResponseError);
}

TEST_CASE("No minor version in HTTP version", "[parsing]")
{
    std::string str = "HTTP/1.";
    REQUIRE_THROWS_AS(http::detail::parseHttpVersion(str.begin(), str.end()), http::ResponseError);
}

TEST_CASE("Parse status code", "[parsing]")
{
    std::string str = "333";
    auto result = http::detail::parseStatusCode(str.begin(), str.end());
    REQUIRE(result.first == str.end());
    REQUIRE(result.second == 333);
}

TEST_CASE("Too short status code", "[parsing]")
{
    std::string str = "33";
    REQUIRE_THROWS_AS(http::detail::parseStatusCode(str.begin(), str.end()), http::ResponseError);
}

TEST_CASE("Invalid short status code", "[parsing]")
{
    std::string str = "33a";
    REQUIRE_THROWS_AS(http::detail::parseStatusCode(str.begin(), str.end()), http::ResponseError);
}

TEST_CASE("Parse reason phrase", "[parsing]")
{
    std::string str = "reason";
    auto result = http::detail::parseReasonPhrase(str.begin(), str.end());
    REQUIRE(result.first == str.end());
    REQUIRE(result.second == "reason");
}

TEST_CASE("Parse reason phrase with space", "[parsing]")
{
    std::string str = "reason s";
    auto result = http::detail::parseReasonPhrase(str.begin(), str.end());
    REQUIRE(result.first == str.end());
    REQUIRE(result.second == "reason s");
}

TEST_CASE("Parse status", "[parsing]")
{
    std::string str = "HTTP/1.1 123 test\r\n";
    auto result = http::detail::parseStatusLine(str.begin(), str.end());
    REQUIRE(result.first == str.end());
    REQUIRE(result.second.httpVersion.major == 1);
    REQUIRE(result.second.httpVersion.minor == 1);
    REQUIRE(result.second.code == 123);
    REQUIRE(result.second.reason == "test");
}

TEST_CASE("Parse field value", "[parsing]")
{
    std::string str = "value";
    auto result = http::detail::parseFieldValue(str.begin(), str.end());
    REQUIRE(result.first == str.end());
    REQUIRE(result.second == "value");
}

TEST_CASE("Parse field value with a space", "[parsing]")
{
    std::string str = "value s";
    auto result = http::detail::parseFieldValue(str.begin(), str.end());
    REQUIRE(result.first == str.end());
    REQUIRE(result.second == "value s");
}

TEST_CASE("Parse field value with trailing whitespaces", "[parsing]")
{
    std::string str = "value \t";
    auto result = http::detail::parseFieldValue(str.begin(), str.end());
    REQUIRE(result.first == str.end());
    REQUIRE(result.second == "value");
}

TEST_CASE("Parse field content", "[parsing]")
{
    std::string str = "content";
    auto result = http::detail::parseFieldContent(str.begin(), str.end());
    REQUIRE(result.first == str.end());
    REQUIRE(result.second == "content");
}

TEST_CASE("Parse field content with obsolete folding", "[parsing]")
{
    std::string str = "content\r\n t";
    auto result = http::detail::parseFieldContent(str.begin(), str.end());
    REQUIRE(result.first == str.end());
    REQUIRE(result.second == "content t");
}

TEST_CASE("Parse field content with obsolete folding and whitespace", "[parsing]")
{
    std::string str = "content\r\n  t";
    auto result = http::detail::parseFieldContent(str.begin(), str.end());
    REQUIRE(result.first == str.end());
    REQUIRE(result.second == "content  t");
}

TEST_CASE("Parse field content with obsolete folding with empty first line", "[parsing]")
{
    std::string str = "\r\n t";
    auto result = http::detail::parseFieldContent(str.begin(), str.end());
    REQUIRE(result.first == str.end());
    REQUIRE(result.second == " t");
}

TEST_CASE("Parse header field", "[parsing]")
{
    std::string str = "field:value\r\n";
    auto result = http::detail::parseHeaderField(str.begin(), str.end());
    REQUIRE(result.first == str.end());
    REQUIRE(result.second.first == "field");
    REQUIRE(result.second.second == "value");
}

TEST_CASE("Parse header field upper case", "[parsing]")
{
    std::string str = "Field:Value\r\n";
    auto result = http::detail::parseHeaderField(str.begin(), str.end());
    REQUIRE(result.first == str.end());
    REQUIRE(result.second.first == "Field");
    REQUIRE(result.second.second == "Value");
}

TEST_CASE("Parse header field with spaces", "[parsing]")
{
    std::string str = "field:value s\r\n";
    auto result = http::detail::parseHeaderField(str.begin(), str.end());
    REQUIRE(result.first == str.end());
    REQUIRE(result.second.first == "field");
    REQUIRE(result.second.second == "value s");
}

TEST_CASE("Parse header field with spaces after colon", "[parsing]")
{
    std::string str = "field:  \tvalue\r\n";
    auto result = http::detail::parseHeaderField(str.begin(), str.end());
    REQUIRE(result.first == str.end());
    REQUIRE(result.second.first == "field");
    REQUIRE(result.second.second == "value");
}

TEST_CASE("Parse header field with no value", "[parsing]")
{
    std::string str = "field:\r\n";
    auto result = http::detail::parseHeaderField(str.begin(), str.end());
    REQUIRE(result.first == str.end());
    REQUIRE(result.second.first == "field");
    REQUIRE(result.second.second == "");
}

TEST_CASE("Parse header field with trailing whitespace", "[parsing]")
{
    std::string str = "field:value \r\n";
    auto result = http::detail::parseHeaderField(str.begin(), str.end());
    REQUIRE(result.first == str.end());
    REQUIRE(result.second.first == "field");
    REQUIRE(result.second.second == "value");
}

TEST_CASE("Parse header field with no colon", "[parsing]")
{
    std::string str = "field\r\n";
    REQUIRE_THROWS_AS(http::detail::parseHeaderField(str.begin(), str.end()), http::ResponseError);
}

TEST_CASE("Parse header field without CRLF", "[parsing]")
{
    std::string str = "field:value";
    REQUIRE_THROWS_AS(http::detail::parseHeaderField(str.begin(), str.end()), http::ResponseError);
}

TEST_CASE("Parse header field with obsolete fold", "[parsing]")
{
    std::string str = "field:value1\r\n value2\r\n";
    auto result = http::detail::parseHeaderField(str.begin(), str.end());
    REQUIRE(result.first == str.end());
    REQUIRE(result.second.first == "field");
    REQUIRE(result.second.second == "value1 value2");
}

TEST_CASE("Create status line", "[serialization]")
{
    std::string result = http::detail::encodeStatusLine("GET", "/");
    REQUIRE(result == "GET / HTTP/1.1\r\n");
}
