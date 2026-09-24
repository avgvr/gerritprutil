// Copyright (c) 2026, Alexey Gavrilov


#pragma once

#include <vector>
#include <stdexcept>
#include <functional>

#include "lexer.h"

struct argument
{
    std::string name, value;
    TokenType valueType;
};

using Argument = std::pair<std::string, std::string>;
using Arguments = std::vector<Argument>;

struct ArgumentTrait
{
    Argument arg;
    TokenType valueType;
    std::function<void(argument&val)> handle;
};

using ArgumentsTrait = std::vector<ArgumentTrait>;

class ArgumentParser
{
private:
    const static ArgumentsTrait ArgsTrait;
    ArgumentLexer lexer;
    token currentToken;
public:
    explicit ArgumentParser(const std::string &args) : lexer(args), currentToken(lexer.tokenize()) {};
    ArgumentParser() = delete;

    argument parseArgument()
    {
        argument opt = {};

        if(currentToken.type == TokenType::T_END_OF_LINE)
        {
            opt.valueType = TokenType::T_END_OF_LINE;
            return opt;
        }

        if(currentToken.type == TokenType::T_SHORT_OPTION
            or currentToken.type == TokenType::T_LONG_OPTION)
        {
            opt.name = currentToken.lexeme;

            currentToken = lexer.tokenize();

            if(currentToken.type == TokenType::T_EQUALS)
            {
                currentToken = lexer.tokenize();

                if(currentToken.type == TokenType::T_NUMBER
                    or currentToken.type == TokenType::T_STRING)
                {
                    opt.value = currentToken.lexeme;
                    opt.valueType = currentToken.type;
                }
                else throw std::runtime_error(
                        "Cannot parse a command line option. Expected a value"
                    );
            }
            else if(currentToken.type == TokenType::T_NUMBER
                    or currentToken.type == TokenType::T_STRING)
            {
                opt.value = currentToken.lexeme;
                opt.valueType = currentToken.type;
            }
            else if(currentToken.type == TokenType::T_SHORT_OPTION
                    or currentToken.type == TokenType::T_LONG_OPTION
                    or currentToken.type == TokenType::T_END_OF_LINE)
            {
                opt.value = "";
                opt.valueType = TokenType::T_VOID;
            }
            else throw std::runtime_error(
                    "Cannot parse a comand line option. Unexpected symbol is reached"
                );
        }
        else throw std::runtime_error("Option is expected");

        return opt;
    };

    ArgumentsTrait::const_iterator verifyArgument(const argument &opt)
    {
        for(ArgumentsTrait::const_iterator argti = ArgsTrait.begin();
            argti != ArgsTrait.end();
            argti++
        )
        {
            const ArgumentTrait &argt = *argti;
            if((argt.arg.first == opt.name
                or argt.arg.second == opt.name)
                and argt.valueType == opt.valueType) return argti;
        }

        return ArgsTrait.end();
    };

    void parseCommandLine()
    {
        for(argument arg = this->parseArgument();
            arg.valueType != TokenType::T_END_OF_LINE;
            arg = this->parseArgument()
        )
        {
            auto trait = verifyArgument(arg);
            if(trait != ArgsTrait.end())
                trait->handle(arg);
            else throw std::runtime_error("Invalid option: " + arg.name + " with val " + "\"" + arg.value + "\"");
        }
    };
};
