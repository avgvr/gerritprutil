// Copyright (c) 2026, Alexey Gavrilov


#include "lexer.h"

void ArgumentLexer::skipWhitespaces()
{
    while(this->curpos < this->cmdln.end()
        and *this->curpos == ' ') 
    {
        this->curpos++;
    }
};

char ArgumentLexer::letterForwards(std::string& lexeme)
{
    auto& cur = this->curpos;
    if(cur < this->cmdln.end())
    {
        lexeme.push_back(*cur++);
        return *cur;
    }
    return cur >= this->cmdln.end() ? '\0' : *cur;
};

Token ArgumentLexer::tokenize()
{
    std::string lexeme;
    while(this->curpos < this->cmdln.end())
    {
        this->skipWhitespaces();

        char ch = *this->curpos;
        TokenType token;
        switch(ch)
        {
            case '-':
                token = TokenType::T_SHORT_OPTION;
                ch = this->letterForwards(lexeme);
                if(ch == '-')
                {
                    token = TokenType::T_LONG_OPTION;
                    ch = this->letterForwards(lexeme);
                }
                if(token == TokenType::T_LONG_OPTION)
                {
                    while(std::islower(ch)) ch = this->letterForwards(lexeme);
                }
                else
                {
                    if(!std::islower(ch)) token = TokenType::T_UNKNOWN;
                    ch = this->letterForwards(lexeme);
                }
                if(!(ch == ' ' or ch == '=' or ch == '\0'))
                {
                    token = TokenType::T_UNKNOWN;
                }
                return {token, lexeme};
            case '=':
                this->letterForwards(lexeme);
                return {TokenType::T_EQUALS, lexeme};
            default:
                if(std::isdigit(ch))
                {
                    do
                    {
                        ch = this->letterForwards(lexeme);
                    }while(std::isdigit(ch));
                    token = TokenType::T_NUMBER;
                    if(!(ch == ' ' or ch == '\0'))
                    {
                        ch = this->letterForwards(lexeme);
                        token = TokenType::T_UNKNOWN;
                    }
                }
                else
                {
                    char bracket = '\0';
                    if(ch == '\"' or ch == '\'') bracket = ch;

                    do{
                        ch = this->letterForwards(lexeme);
                    }while(!(ch == '\"' or ch == '\'' or ch == '\0')
                            and (bracket or ch != ' '));
                    token = TokenType::T_STRING;
                    if(bracket and ch != bracket) token = TokenType::T_UNKNOWN;
                    else if(bracket) ch = this->letterForwards(lexeme);
                }
                return {token, lexeme};
        }
    }
    return {TokenType::T_END_OF_LINE, ""};
};

ArgumentLexer& ArgumentLexer::operator=(const ArgumentLexer& le)
{
    this->cmdln = le.cmdln;
    this->curpos = cmdln.begin() + (le.curpos - le.cmdln.begin());

    return *this;
};
