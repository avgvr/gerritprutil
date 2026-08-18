// Copyright (c) 2026, Alexey Gavrilov


#pragma once

#include <iostream>
#include <string>


enum class TokenType
{
    T_SHORT_OPTION,
    T_LONG_OPTION,
    T_EQUALS,

    // Option value type
    T_STRING,
    T_NUMBER,

    T_UNKNOWN,

    T_END_OF_LINE
};

struct Token
{
    TokenType type;
    std::string lexeme;
};

class ArgumentLexer
{
    std::string cmdln;
    std::string::const_iterator curpos;

public:
    explicit ArgumentLexer(const std::string args)
        : cmdln(args), curpos(cmdln.begin())
    {};
    ArgumentLexer(const ArgumentLexer& lc)
        : cmdln(lc.cmdln),
        curpos(cmdln.begin() + (lc.curpos - lc.cmdln.begin())) {};
    ArgumentLexer(ArgumentLexer&&) = default;
    ArgumentLexer() = delete;

    char letterForwards(std::string& lexeme);
    void skipWhitespaces();
    Token tokenize();

    ArgumentLexer& operator=(const ArgumentLexer& le);
};
