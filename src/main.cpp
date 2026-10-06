// Copyright (c) 2026, Alexey Gavrilov

#include "parser.h"
#include "socket.h"
#include "workers.h"

#include <iostream>

#include <sys/time.h>

void help(argument &o){std::cout << "help func" << std::endl; exit(0);}
void version(argument &o){std::cout << VERSION << std::endl; exit(0);}


const ArgumentsTrait ArgumentParser::ArgsTrait = {
    {{"--help", "-h"}, TokenType::T_VOID , help},
    {{"--version", "-v"}, TokenType::T_VOID, version},
    {{"--int", "-i"}, TokenType::T_NUMBER, version}
};

using Socket = sock::SocketConnector<sock::socktype::stream, sock::domain::inet>;
using SocketAddress = Socket::Address;

int main(int argc, char* argv[])
{
    std::string cmdln = "";
    for(int i = 1; i < argc ; i++)
        cmdln += (i > 1 ? " " : "") + std::string(argv[i]);

    ArgumentParser p(cmdln);
    p.parseCommandLine();
    Socket sock;
    SocketAddress addr;
    addr.setPlacement("0.0.0.0", 31024);

    int trueint = 1;
    sock.setSocketOption(sock::Protocol::api, sock::SocketOption::reuseaddr, trueint);

    struct timeval timeout = {2, 0};
    sock.setSocketOption(sock::Protocol::api, sock::SocketOption::rcvtimeo, timeout);

    sock.bind(addr);

    Transceiver t(sock);
    HttpHandler http;

    worker(http, t);
};
