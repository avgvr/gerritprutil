// Copyright (c) 2026, Alexey Gavrilov


#include <string>
#include <deque>

#include <gtest/gtest.h>

#include "../../src/lexer.h"


TEST(LexerTestingExpectedWork, EmptyCommandLineArguments)
{
    ArgumentLexer l("");
    Token t = l.tokenize();

    EXPECT_EQ(t.type, TokenType::T_END_OF_LINE);
    EXPECT_EQ(t.lexeme, std::string(""));
}

TEST(LexerTestingExpectedWork, ShortCommands)
{
    ArgumentLexer l("-s");
    Token t = l.tokenize();

    EXPECT_EQ(t.type, TokenType::T_SHORT_OPTION);
    EXPECT_EQ(t.lexeme, std::string("-s"));

    l = ArgumentLexer("-s        ");
    t = l.tokenize();

    EXPECT_EQ(t.type, TokenType::T_SHORT_OPTION);
    EXPECT_EQ(t.lexeme, std::string("-s"));

    l = ArgumentLexer("-s=");
    t = l.tokenize();

    EXPECT_EQ(t.type, TokenType::T_SHORT_OPTION);
    EXPECT_EQ(t.lexeme, std::string("-s"));

    l = ArgumentLexer("             -s");
    t = l.tokenize();

    EXPECT_EQ(t.type, TokenType::T_SHORT_OPTION);
    EXPECT_EQ(t.lexeme, std::string("-s"));
}

TEST(LexerTestingExpectedWork, LongCommands)
{
    ArgumentLexer l = ArgumentLexer("--long");
    Token t = l.tokenize();

    EXPECT_EQ(t.type, TokenType::T_LONG_OPTION);
    EXPECT_EQ(t.lexeme, std::string("--long"));

    l = ArgumentLexer("--long        ");
    t = l.tokenize();

    EXPECT_EQ(t.type, TokenType::T_LONG_OPTION);
    EXPECT_EQ(t.lexeme, std::string("--long"));

    l = ArgumentLexer("--long=");
    t = l.tokenize();

    EXPECT_EQ(t.type, TokenType::T_LONG_OPTION);
    EXPECT_EQ(t.lexeme, std::string("--long"));

    l = ArgumentLexer("             --long");
    t = l.tokenize();

    EXPECT_EQ(t.type, TokenType::T_LONG_OPTION);
    EXPECT_EQ(t.lexeme, std::string("--long"));
}

TEST(LexerTestingExpectedWork, EqualsToken)
{
    ArgumentLexer l = ArgumentLexer("=");
    Token t = l.tokenize();

    EXPECT_EQ(t.type, TokenType::T_EQUALS);
    EXPECT_EQ(t.lexeme, std::string("="));
}

TEST(LexerTestingExpectedWork, StringValues)
{
    ArgumentLexer l = ArgumentLexer("\"string\"");
    Token t = l.tokenize();

    EXPECT_EQ(t.type, TokenType::T_STRING);
    EXPECT_EQ(t.lexeme, std::string("\"string\""));

    l = ArgumentLexer("\'string\'");
    t = l.tokenize();

    EXPECT_EQ(t.type, TokenType::T_STRING);
    EXPECT_EQ(t.lexeme, std::string("\'string\'"));

    l = ArgumentLexer("string");
    t = l.tokenize();

    EXPECT_EQ(t.type, TokenType::T_STRING);
    EXPECT_EQ(t.lexeme, std::string("string"));

    l = ArgumentLexer("!@#$%^&*()№;:?\"\'\\/+-*");
    t = l.tokenize();

    EXPECT_EQ(t.type, TokenType::T_STRING);
    EXPECT_EQ(t.lexeme, std::string("!@#$%^&*()№;:?"));

    l = ArgumentLexer("\"string1 string2\"");
    t = l.tokenize();

    EXPECT_EQ(t.type, TokenType::T_STRING);
    EXPECT_EQ(t.lexeme, std::string("\"string1 string2\""));

    l = ArgumentLexer("string                   ");
    t = l.tokenize();

    EXPECT_EQ(t.type, TokenType::T_STRING);
    EXPECT_EQ(t.lexeme, std::string("string"));

    l = ArgumentLexer("string\"                   ");
    t = l.tokenize();

    EXPECT_EQ(t.type, TokenType::T_STRING);
    EXPECT_EQ(t.lexeme, std::string("string"));
}

TEST(LexerTestingExpectedWork, NumberValues)
{
    ArgumentLexer l = ArgumentLexer("1232123");
    Token t = l.tokenize();

    EXPECT_EQ(t.type, TokenType::T_NUMBER);
    EXPECT_EQ(t.lexeme, std::string("1232123"));

    l = ArgumentLexer("1111   ");
    t = l.tokenize();

    EXPECT_EQ(t.type, TokenType::T_NUMBER);
    EXPECT_EQ(t.lexeme, std::string("1111"));
};

TEST(LexerTestingExpectedWork, WholeArguments)
{
    ArgumentLexer l = ArgumentLexer("-h helpstring --count 5");

    std::deque<Token> tkns =
    {
        {TokenType::T_SHORT_OPTION, "-h"},
        {TokenType::T_STRING, "helpstring"},
        {TokenType::T_LONG_OPTION, "--count"},
        {TokenType::T_NUMBER, "5"}
    };

    Token t, tf;

    t = l.tokenize();
    while(!tkns.empty())
    {
        tf = tkns.front(); tkns.pop_front();
        EXPECT_EQ(t.type, tf.type);
        EXPECT_EQ(t.lexeme, tf.lexeme);
        t = l.tokenize();
    };
        EXPECT_EQ(t.type, TokenType::T_END_OF_LINE);
        EXPECT_EQ(t.lexeme, "");
};
