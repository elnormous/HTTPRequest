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

TEST_CASE("Tchar", "[parsing]")
{
    for (int c = 0; c < 256; ++c)
        REQUIRE(http::detail::isVisibleChar(static_cast<char>(c)) == (c >= 0x21 && c <= 0x7E));
}

TEST_CASE("Vchar", "[parsing]")
{
    for (int c = 0; c < 256; ++c)
        REQUIRE(http::detail::isTokenChar(static_cast<char>(c)) ==
                (c >= 0x21 && c <= 0x7E &&
                 c != '"' && c != '(' && c != ')' && c != ',' && c != '/' &&
                 c != ':' && c != ';' && c != '<' && c != '=' && c != '>' &&
                 c != '?' && c != '@' && c != '[' && c != '\\' && c != ']' &&
                 c != '{' && c != '}'));
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
