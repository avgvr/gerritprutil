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
                ch = this->letterForwards(lexeme);
                token = TokenType::T_SHORT_OPTION;
                if(ch == '-')
                {
                    token = TokenType::T_LONG_OPTION;
                    ch = this->letterForwards(lexeme);
                }
                while(std::islower(ch)) ch = this->letterForwards(lexeme);
                if(!(ch == ' ' || ch == '='))
                {
                    token = TokenType::T_UNKNOWN;
                }
                return {token, lexeme};
            case '=':
                this->letterForwards(lexeme);
                return {TokenType::T_EQUAL, lexeme};
            default:
                if(std::isdigit(ch))
                {
                    do
                    {
                        ch = this->letterForwards(lexeme);
                    }while(std::isdigit(ch));
                    token = TokenType::T_NUMBER;
                    if(ch != ' ') token = TokenType::T_UNKNOWN;
                }
                else
                {
                    char openStrCh = '\0';
                    if(ch == '\"' or ch == '\'') openStrCh = ch;

                    do{
                        ch = this->letterForwards(lexeme);
                    }while(ch != openStrCh and ch != '\0');
                    token = TokenType::T_STRING;
                    if(ch != ' ') token = TokenType::T_UNKNOWN;
                }
                return {token, lexeme};
        }
    }
    return {TokenType::T_END_OF_LINE, ""};
};
