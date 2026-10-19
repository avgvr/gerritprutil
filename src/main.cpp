// Copyright (c) 2026, Alexey Gavrilov

#include "parser.h"
#include "socket.h"
#include "workers.h"

#include <iostream>

#include <sys/time.h>

using EncryptedPassiveSocket =
    sock::EncryptedPassiveSocket<
        sock::socktype::stream,
        sock::domain::inet
    >;


static std::string CHAINFILE = EncryptedPassiveSocket::DefaultChainFile;
static std::string KEYFILE = EncryptedPassiveSocket::DefaultPrivateFile;

void help_cb(argument &o){std::cout << "help func" << std::endl; exit(0);};
void version_cb(argument &o){std::cout << VERSION << std::endl; exit(0);};
void keyfile_cb(argument &o){CHAINFILE = o.value;};
void chainfile_cb(argument &o){KEYFILE = o.value;};


const ArgumentsTrait ArgumentParser::ArgsTrait = {
    {{"--help", "-h"}, TokenType::T_VOID , help_cb},
    {{"--version", "-v"}, TokenType::T_VOID, version_cb},
    {{"--key-file", "-k"}, TokenType::T_STRING, keyfile_cb},
    {{"--chain-file", "-c"}, TokenType::T_STRING, chainfile_cb},
    {{"--gerritaddress", "-g"}, TokenType::T_STRING, gerritaddress_cb},
    {{"--gerritusername", "-u"}, TokenType::T_STRING, gerritusername_cb},
    {{"--gerritpassword", "-a"}, TokenType::T_STRING, gerritpassword_cb},
    {{"--branch", "-b"}, TokenType::T_STRING, pushbranch_pr},
    {{"--github-app-tokenid", "-t"}, TokenType::T_STRING, githubapptokenid_cb},
    {{"--github-key-path", "-p"}, TokenType::T_STRING, githubkeypath_cb},
};

using Socket = sock::EncryptedSocketConnector<sock::socktype::stream, sock::domain::inet>;
using SocketAddress = Socket::Address;

const static in_port_t Port = 31013;

int main(int argc, char **argv)
{
    std::ostringstream cmdln;
    std::string delimiter = "";
    for(int i = 1; i < argc ; i++)
    {
        std::string arg(argv[i]);
        cmdln << delimiter;
        if(arg.find(' ') != std::string::npos)
            cmdln << std::quoted(arg);
        else cmdln << arg;

        delimiter = " ";
    }

    ArgumentParser p(cmdln.str());
    p.parseCommandLine();
    Socket sock({CHAINFILE, KEYFILE});
    SocketAddress addr;
    addr.setPlacement("0.0.0.0", Port);
    sock.bind(addr);

    constexpr static int trueint = 1;
    sock.setSocketOption(sock::Protocol::api, sock::SocketOption::reuseaddr, trueint);

    constexpr static struct timeval timeout = {2, 0};
    sock.setSocketOption(sock::Protocol::api, sock::SocketOption::rcvtimeo, timeout);

    sock.listen(4);

    HttpHandler http;

    worker(http, sock);
};
