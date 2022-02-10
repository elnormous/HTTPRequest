#include <cstddef>
#include "catch2/catch.hpp"
#include "HTTPRequest.hpp"

TEST_CASE("Whitespace", "[parsing]")
{
    REQUIRE(http::detail::isWhitespace(' '));
    REQUIRE(http::detail::isWhitespace('\t'));
}

TEST_CASE("Tchar", "[parsing]")
{
    for (int c = 0; c < 256; ++c)
        REQUIRE(http::detail::isVchar(static_cast<char>(c)) == (c >= 0x21 && c <= 0x7E));
}

TEST_CASE("Vchar", "[parsing]")
{
    for (int c = 0; c < 256; ++c)
        REQUIRE(http::detail::isTchar(static_cast<char>(c)) ==
                (c >= 0x21 && c <= 0x7E &&
                 c != '"' && c != '(' && c != ')' && c != ',' && c != '/' &&
                 c != ':' && c != ';' && c != '<' && c != '=' && c != '>' &&
                 c != '?' && c != '@' && c != '[' && c != '\\' && c != ']' &&
                 c != '{' && c != '}'));
}
