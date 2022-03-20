#include <cstddef>
#include "catch2/catch.hpp"
#include "HTTPRequest.hpp"

TEST_CASE("White space", "[parsing]")
{
    for (int c = 0; c < 256; ++c)
        REQUIRE(http::isWhiteSpaceChar(static_cast<char>(c)) == (c == ' ' || c == '\t'));
}

TEST_CASE("Digit", "[parsing]")
{
    for (int c = 0; c < 256; ++c)
        REQUIRE(http::isDigitChar(static_cast<char>(c)) == (c >= '0' && c <= '9'));
}

TEST_CASE("Alpha", "[parsing]")
{
    for (int c = 0; c < 256; ++c)
        REQUIRE(http::isAlphaChar(static_cast<char>(c)) == ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')));
}

TEST_CASE("Token char", "[parsing]")
{
    for (int c = 0; c < 256; ++c)
        REQUIRE(http::isTokenChar(static_cast<char>(c)) ==
                (c >= 0x21 && c <= 0x7E &&
                 c != '"' && c != '(' && c != ')' && c != ',' && c != '/' &&
                 c != ':' && c != ';' && c != '<' && c != '=' && c != '>' &&
                 c != '?' && c != '@' && c != '[' && c != '\\' && c != ']' &&
                 c != '{' && c != '}'));
}

TEST_CASE("Visible char", "[parsing]")
{
    for (int c = 0; c < 256; ++c)
        REQUIRE(http::isVisibleChar(static_cast<char>(c)) == (c >= 0x21 && c <= 0x7E));
}

TEST_CASE("OBS text char", "[parsing]")
{
    for (int c = 0; c < 256; ++c)
        REQUIRE(http::isObsoleteTextChar(static_cast<char>(c)) == (c >= 0x80 && c <= 0xFF));
}

TEST_CASE("Skip empty whites paces", "[parsing]")
{
    const std::string str = "";
    const auto i = http::skipWhiteSpaces(str.begin(), str.end());
    REQUIRE(i == str.begin());
    REQUIRE(i == str.end());
}

TEST_CASE("Skip one white space", "[parsing]")
{
    const std::string str = " ";
    const auto i = http::skipWhiteSpaces(str.begin(), str.end());
    REQUIRE(i == str.end());
}

TEST_CASE("Skip one white space at the beggining", "[parsing]")
{
    const std::string str = " a";
    const auto i = http::skipWhiteSpaces(str.begin(), str.end());
    REQUIRE(i == str.begin() + 1);
}

TEST_CASE("Don't skip white spaces", "[parsing]")
{
    const std::string str = "a ";
    const auto i = http::skipWhiteSpaces(str.begin(), str.end());
    REQUIRE(i == str.begin());
}

TEST_CASE("Parse token", "[parsing]")
{
    const std::string str = "token";
    const auto result = http::parseToken(str.begin(), str.end());
    REQUIRE(result.first == str.end());
    REQUIRE(result.second == "token");
}

TEST_CASE("Parse HTTP version", "[parsing]")
{
    const std::string str = "HTTP/1.1";
    const auto result = http::parseHttpVersion(str.begin(), str.end());
    REQUIRE(result.first == str.end());
    REQUIRE(result.second.major == 1U);
    REQUIRE(result.second.minor == 1U);
}

TEST_CASE("Invalid HTTP in version", "[parsing]")
{
    const std::string str = "TTP/1.1";
    REQUIRE_THROWS_AS(http::parseHttpVersion(str.begin(), str.end()), http::ResponseError);
}

TEST_CASE("No slash in HTTP version", "[parsing]")
{
    const std::string str = "HTTP1.1";
    REQUIRE_THROWS_AS(http::parseHttpVersion(str.begin(), str.end()), http::ResponseError);
}

TEST_CASE("No minor version in HTTP version", "[parsing]")
{
    const std::string str = "HTTP/1.";
    REQUIRE_THROWS_AS(http::parseHttpVersion(str.begin(), str.end()), http::ResponseError);
}

TEST_CASE("Parse status code", "[parsing]")
{
    const std::string str = "333";
    const auto result = http::parseStatusCode(str.begin(), str.end());
    REQUIRE(result.first == str.end());
    REQUIRE(result.second == 333);
}

TEST_CASE("Too short status code", "[parsing]")
{
    const std::string str = "33";
    REQUIRE_THROWS_AS(http::parseStatusCode(str.begin(), str.end()), http::ResponseError);
}

TEST_CASE("Too long status code", "[parsing]")
{
    const std::string str = "3333";
    REQUIRE_THROWS_AS(http::parseStatusCode(str.begin(), str.end()), http::ResponseError);
}

TEST_CASE("Invalid status code", "[parsing]")
{
    const std::string str = "33a";
    REQUIRE_THROWS_AS(http::parseStatusCode(str.begin(), str.end()), http::ResponseError);
}

TEST_CASE("Parse reason phrase", "[parsing]")
{
    const std::string str = "reason";
    const auto result = http::parseReasonPhrase(str.begin(), str.end());
    REQUIRE(result.first == str.end());
    REQUIRE(result.second == "reason");
}

TEST_CASE("Parse reason phrase with space", "[parsing]")
{
    const std::string str = "reason s";
    const auto result = http::parseReasonPhrase(str.begin(), str.end());
    REQUIRE(result.first == str.end());
    REQUIRE(result.second == "reason s");
}

TEST_CASE("Parse status", "[parsing]")
{
    const std::string str = "HTTP/1.1 123 test\r\n";
    const auto result = http::parseStatusLine(str.begin(), str.end());
    REQUIRE(result.first == str.end());
    REQUIRE(result.second.httpVersion.major == 1);
    REQUIRE(result.second.httpVersion.minor == 1);
    REQUIRE(result.second.code == 123);
    REQUIRE(result.second.reason == "test");
}

TEST_CASE("Parse field value", "[parsing]")
{
    const std::string str = "value";
    const auto result = http::parseFieldValue(str.begin(), str.end());
    REQUIRE(result.first == str.end());
    REQUIRE(result.second == "value");
}

TEST_CASE("Parse field value with a space", "[parsing]")
{
    const std::string str = "value s";
    const auto result = http::parseFieldValue(str.begin(), str.end());
    REQUIRE(result.first == str.end());
    REQUIRE(result.second == "value s");
}

TEST_CASE("Parse field value with trailing white spaces", "[parsing]")
{
    const std::string str = "value \t";
    const auto result = http::parseFieldValue(str.begin(), str.end());
    REQUIRE(result.first == str.end());
    REQUIRE(result.second == "value");
}

TEST_CASE("Parse field content", "[parsing]")
{
    const std::string str = "content";
    const auto result = http::parseFieldContent(str.begin(), str.end());
    REQUIRE(result.first == str.end());
    REQUIRE(result.second == "content");
}

TEST_CASE("Parse field content with obsolete folding", "[parsing]")
{
    const std::string str = "content\r\n t";
    const auto result = http::parseFieldContent(str.begin(), str.end());
    REQUIRE(result.first == str.end());
    REQUIRE(result.second == "content t");
}

TEST_CASE("Parse field content with obsolete folding and white space", "[parsing]")
{
    const std::string str = "content\r\n  t";
    const auto result = http::parseFieldContent(str.begin(), str.end());
    REQUIRE(result.first == str.end());
    REQUIRE(result.second == "content  t");
}

TEST_CASE("Parse field content with obsolete folding with empty first line", "[parsing]")
{
    const std::string str = "\r\n t";
    const auto result = http::parseFieldContent(str.begin(), str.end());
    REQUIRE(result.first == str.end());
    REQUIRE(result.second == " t");
}

TEST_CASE("Parse header field", "[parsing]")
{
    const std::string str = "field:value\r\n";
    const auto result = http::parseHeaderField(str.begin(), str.end());
    REQUIRE(result.first == str.end());
    REQUIRE(result.second.first == "field");
    REQUIRE(result.second.second == "value");
}

TEST_CASE("Parse header field upper case", "[parsing]")
{
    const std::string str = "Field:Value\r\n";
    const auto result = http::parseHeaderField(str.begin(), str.end());
    REQUIRE(result.first == str.end());
    REQUIRE(result.second.first == "Field");
    REQUIRE(result.second.second == "Value");
}

TEST_CASE("Parse header field with spaces", "[parsing]")
{
    const std::string str = "field:value s\r\n";
    const auto result = http::parseHeaderField(str.begin(), str.end());
    REQUIRE(result.first == str.end());
    REQUIRE(result.second.first == "field");
    REQUIRE(result.second.second == "value s");
}

TEST_CASE("Parse header field with spaces after colon", "[parsing]")
{
    const std::string str = "field:  \tvalue\r\n";
    const auto result = http::parseHeaderField(str.begin(), str.end());
    REQUIRE(result.first == str.end());
    REQUIRE(result.second.first == "field");
    REQUIRE(result.second.second == "value");
}

TEST_CASE("Parse header field with no value", "[parsing]")
{
    const std::string str = "field:\r\n";
    auto result = http::parseHeaderField(str.begin(), str.end());
    REQUIRE(result.first == str.end());
    REQUIRE(result.second.first == "field");
    REQUIRE(result.second.second == "");
}

TEST_CASE("Parse header field with trailing white space", "[parsing]")
{
    const std::string str = "field:value \r\n";
    auto result = http::parseHeaderField(str.begin(), str.end());
    REQUIRE(result.first == str.end());
    REQUIRE(result.second.first == "field");
    REQUIRE(result.second.second == "value");
}

TEST_CASE("Parse header field with no colon", "[parsing]")
{
    const std::string str = "field\r\n";
    REQUIRE_THROWS_AS(http::parseHeaderField(str.begin(), str.end()), http::ResponseError);
}

TEST_CASE("Parse header with missing line feed", "[parsing]")
{
    const std::string str = "a:b\rc:d\r\n";
    REQUIRE_THROWS_AS(http::parseHeaderField(str.begin(), str.end()), http::ResponseError);
}

TEST_CASE("Parse header with missing carriage return", "[parsing]")
{
    const std::string str = "a:b\nc:d\r\n";
    REQUIRE_THROWS_AS(http::parseHeaderField(str.begin(), str.end()), http::ResponseError);
}

TEST_CASE("Parse header field without CRLF", "[parsing]")
{
    const std::string str = "field:value";
    REQUIRE_THROWS_AS(http::parseHeaderField(str.begin(), str.end()), http::ResponseError);
}

TEST_CASE("Parse header field with obsolete fold", "[parsing]")
{
    const std::string str = "field:value1\r\n value2\r\n";
    const auto result = http::parseHeaderField(str.begin(), str.end());
    REQUIRE(result.first == str.end());
    REQUIRE(result.second.first == "field");
    REQUIRE(result.second.second == "value1 value2");
}

TEST_CASE("Digit to unsigned int", "[parsing]")
{
    const char c = '1';
    REQUIRE(http::digitToUint<std::size_t>(c) == 1U);
}

TEST_CASE("Invalid digit", "[parsing]")
{
    const char c = 'a';
    REQUIRE_THROWS_AS(http::digitToUint<std::size_t>(c), http::ResponseError);
}

TEST_CASE("Digits to unsigned int", "[parsing]")
{
    const std::string str = "11";
    REQUIRE(http::stringToUint<std::size_t>(str.begin(), str.end()) == 11U);
}

TEST_CASE("Invalid digit string", "[parsing]")
{
    const std::string str = "1x";
    REQUIRE_THROWS_AS(http::stringToUint<std::size_t>(str.begin(), str.end()), http::ResponseError);
}

TEST_CASE("Hex digit and letter to unsigned int", "[parsing]")
{
    const std::string str = "1A";
    REQUIRE(http::hexStringToUint<std::size_t>(str.begin(), str.end()) == 26U);
}

TEST_CASE("Hex digit to unsigned int", "[parsing]")
{
    const char c = '1';
    REQUIRE(http::hexDigitToUint<std::size_t>(c) == 1U);
}

TEST_CASE("Hex lowercase letter to unsigned int", "[parsing]")
{
    const char c = 'a';
    REQUIRE(http::hexDigitToUint<std::size_t>(c) == 10U);
}

TEST_CASE("Hex uppercase letter to unsigned int", "[parsing]")
{
    const char c = 'A';
    REQUIRE(http::hexDigitToUint<std::size_t>(c) == 10U);
}

TEST_CASE("Invalid hex", "[parsing]")
{
    const char c = 'x';
    REQUIRE_THROWS_AS(http::hexDigitToUint<std::size_t>(c), http::ResponseError);
}

TEST_CASE("Hex digits with a letter last to unsigned int", "[parsing]")
{
    const std::string str = "1A";
    REQUIRE(http::hexStringToUint<std::size_t>(str.begin(), str.end()) == 26U);
}

TEST_CASE("Hex digits with a letter first to unsigned int", "[parsing]")
{
    const std::string str = "A1";
    REQUIRE(http::hexStringToUint<std::size_t>(str.begin(), str.end()) == 161U);
}

TEST_CASE("Invalid hex string", "[parsing]")
{
    const std::string str = "ax";
    REQUIRE_THROWS_AS(http::hexStringToUint<std::size_t>(str.begin(), str.end()), http::ResponseError);
}

TEST_CASE("Parse URL", "[parsing]")
{
    const std::string str = "tt://www.test.com:80/path";
    const http::Uri uri = http::parseUri(str.begin(), str.end());
    REQUIRE(uri.scheme == "tt");
    REQUIRE(uri.user == "");
    REQUIRE(uri.password == "");
    REQUIRE(uri.host == "www.test.com");
    REQUIRE(uri.port == "80");
    REQUIRE(uri.path == "/path");
    REQUIRE(uri.query == "");
    REQUIRE(uri.fragment == "");
}

TEST_CASE("Parse URL with non-alpha non-digit characters in scheme", "[parsing]")
{
    const std::string str = "t.t+-://foo";
    const http::Uri uri = http::parseUri(str.begin(), str.end());
    REQUIRE(uri.scheme == "t.t+-");
    REQUIRE(uri.host == "foo");
}

TEST_CASE("Parse URL with invalid character in scheme", "[parsing]")
{
    const std::string str = "tt!://foo";
    REQUIRE_THROWS_AS(http::parseUri(str.begin(), str.end()), http::RequestError);
}

TEST_CASE("Parse URL with fragment", "[parsing]")
{
    const std::string str = "tt://www.test.com/path#fragment";
    const http::Uri uri = http::parseUri(str.begin(), str.end());
    REQUIRE(uri.scheme == "tt");
    REQUIRE(uri.user == "");
    REQUIRE(uri.password == "");
    REQUIRE(uri.host == "www.test.com");
    REQUIRE(uri.port == "");
    REQUIRE(uri.path == "/path");
    REQUIRE(uri.query == "");
    REQUIRE(uri.fragment == "fragment");
}

TEST_CASE("Parse URL with query and fragment", "[parsing]")
{
    const std::string str = "tt://www.test.com/path?query=1#fragment";
    const http::Uri uri = http::parseUri(str.begin(), str.end());
    REQUIRE(uri.scheme == "tt");
    REQUIRE(uri.user == "");
    REQUIRE(uri.password == "");
    REQUIRE(uri.host == "www.test.com");
    REQUIRE(uri.port == "");
    REQUIRE(uri.path == "/path");
    REQUIRE(uri.query == "query=1");
    REQUIRE(uri.fragment == "fragment");
}

TEST_CASE("Parse URL without scheme", "[parsing]")
{
    const std::string str = "www.test.com/path?query=1#fragment";
    REQUIRE_THROWS_AS(http::parseUri(str.begin(), str.end()), http::RequestError);
}

TEST_CASE("Parse URL with user", "[parsing]")
{
    const std::string str = "tt://test@test.com/";
    const http::Uri uri = http::parseUri(str.begin(), str.end());
    REQUIRE(uri.scheme == "tt");
    REQUIRE(uri.user == "test");
    REQUIRE(uri.password == "");
    REQUIRE(uri.host == "test.com");
    REQUIRE(uri.port == "");
    REQUIRE(uri.path == "/");
    REQUIRE(uri.query == "");
    REQUIRE(uri.fragment == "");
}

TEST_CASE("Parse URL with user and password", "[parsing]")
{
    const std::string str = "tt://test:test@test.com/";
    const http::Uri uri = http::parseUri(str.begin(), str.end());
    REQUIRE(uri.scheme == "tt");
    REQUIRE(uri.user == "test");
    REQUIRE(uri.password == "test");
    REQUIRE(uri.host == "test.com");
    REQUIRE(uri.port == "");
    REQUIRE(uri.path == "/");
    REQUIRE(uri.query == "");
    REQUIRE(uri.fragment == "");
}
