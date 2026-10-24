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

token ArgumentLexer::tokenize()
{
    std::string lexeme;
    while(this->curpos < this->cmdln.end())
    {
        this->skipWhitespaces();

        char ch = *this->curpos;
        TokenType tkn;
        switch(ch)
        {
            case '-':
                tkn = TokenType::T_SHORT_OPTION;
                ch = this->letterForwards(lexeme);
                if(ch == '-')
                {
                    tkn = TokenType::T_LONG_OPTION;
                    ch = this->letterForwards(lexeme);
                }
                if(tkn == TokenType::T_LONG_OPTION)
                {
                    while(std::islower(ch) or ch == '-') ch = this->letterForwards(lexeme);
                }
                else
                {
                    if(!std::islower(ch)) tkn = TokenType::T_UNKNOWN;
                    ch = this->letterForwards(lexeme);
                }
                if(!(ch == ' ' or ch == '=' or ch == '\0'))
                {
                    tkn = TokenType::T_UNKNOWN;
                }
                return {tkn, lexeme};
            case '=':
                this->letterForwards(lexeme);
                return {TokenType::T_EQUALS, lexeme};
            default:
                while(std::isdigit(ch))
                    ch = this->letterForwards(lexeme);

                tkn = TokenType::T_NUMBER;
                if(ch != ' ' and ch)
                {
                    tkn = TokenType::T_STRING;
                    char bracket = '\0';
                    if(ch == '\'' or ch == '\"') bracket = ch;

                    ch = letterForwards(lexeme);
                    while((ch != ' ' and ch and !bracket) or (bracket and ch != bracket))
                    {
                        if((ch == '\'' or ch == '\"') and ch != bracket)
                        {
                            tkn = TokenType::T_UNKNOWN;
                            break;
                        }
                        else if(ch == '\0') break;
                        ch = letterForwards(lexeme);
                    }

                    if(ch == bracket)
                        ch = letterForwards(lexeme);
                    else if(bracket and ch != bracket)
                        tkn = TokenType::T_UNKNOWN;
                    else if(!bracket and ch != ' ' and ch)
                        tkn = TokenType::T_UNKNOWN;
                }
                return {tkn, lexeme};
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
