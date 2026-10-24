// Copyright (c) 2026, Alexey Gavrilov

#include "parser.h"
#include "socket.h"
#include "workers.h"

#include <iostream>

#include <sys/time.h>

/**
* Commands description string
 */
const static std::string helpDesc = "Display help information";
const static std::string versionDesc = "Return version string";
const static std::string keyfileDesc = "Path to a SSL private key file. Default value is key,pem";
const static std::string chainfileDesc = "Path to a SSL certificate chain file. Default value is cert.pem";
const static std::string gerritaddressDesc = "Address to a Gerrit in format {hostname/IP address}:{port}. Default value is localhost:443";
const static std::string gerritusernameDesc = "Gerrit username to a user with administrator rights";
const static std::string gerritpasswordDesc = "Gerrit user token/http password to an accordingly user specified with gerritusername option";
const static std::string branchDesc = "Base branch name to merge upload commits. Default value is main";
const static std::string githubclientidDesc = "GitHub application client id";
const static std::string githubkeypathDesc = "GitHub application private key file path";

using EncryptedPassiveSocket =
    sock::EncryptedPassiveSocket<
        sock::socktype::stream,
        sock::domain::inet
    >;

static std::string CHAINFILE = EncryptedPassiveSocket::DefaultChainFile;
static std::string KEYFILE = EncryptedPassiveSocket::DefaultPrivateFile;

void help_cb(argument &o)
{
    for(auto &optTrait : ArgumentParser::ArgsTrait)
    {
        const std::string argField =
            optTrait.arg.first + ", " + optTrait.arg.second;

        std::cout
            << std::left
            << std::setw(40)
                << argField
            << std::left
            << std::setw(60)
                << optTrait.description
            << std::endl;
    }
    exit(0);
};
void version_cb(argument &o){std::cout << VERSION << std::endl; exit(0);};
void keyfile_cb(argument &o){KEYFILE = o.value;};
void chainfile_cb(argument &o){CHAINFILE = o.value;};


const ArgumentsTrait ArgumentParser::ArgsTrait = {
    {{"--help", "-h"}, TokenType::T_VOID , help_cb, helpDesc},
    {{"--version", "-v"}, TokenType::T_VOID, version_cb, versionDesc},
    {{"--key-file", "-k"}, TokenType::T_STRING, keyfile_cb, keyfileDesc},
    {{"--chain-file", "-c"}, TokenType::T_STRING, chainfile_cb, chainfileDesc},
    {{"--gerritaddress", "-g"}, TokenType::T_STRING, gerritaddress_cb, gerritaddressDesc},
    {{"--gerritusername", "-u"}, TokenType::T_STRING, gerritusername_cb, gerritusernameDesc},
    {{"--gerritpassword", "-a"}, TokenType::T_STRING, gerritpassword_cb, gerritpasswordDesc},
    {{"--branch", "-b"}, TokenType::T_STRING, pushbranch_pr, branchDesc},
    {{"--github-app-clientid", "-t"}, TokenType::T_STRING, githubapptokenid_cb, githubclientidDesc},
    {{"--github-key-path", "-p"}, TokenType::T_STRING, githubkeypath_cb, githubkeypathDesc},
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
