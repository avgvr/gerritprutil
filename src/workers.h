// Copyright (c) 2026, Alexey Gavrilov


#include "git.h"
#include "handlers.h"
#include "transceiver.h"


template<sock::socktype Type, sock::domain Dom>
void worker(EchoHandler &h, Transceiver<Type, Dom> &r)
{
    h.acceptRequest(r.receivePacket());
    r.closeConnection();
};

template<sock::socktype Type, sock::domain Dom>
void worker(HttpHandler &h, Transceiver<Type, Dom> &r)
{
    h.acceptRequest(r.receivePacket());
    std::vector<unsigned char> response = h.getResponse();
    r.sendPacket(response);
    r.closeConnection();

    if(!h.isBodyValid()) return;

    std::string repopath = "/home/rebovas/projects/test";
    std::vector<std::string> prrefspec = {"+refs/pull/*:refs/pull*"};
    git::Repository repo(repopath);
    repo.fetchRemote(h.repository().first, prrefspec);
    auto cmts = repo.listCommits(h.base(), h.head());
    for(auto &cmt : cmts) std::cout << cmt->summary() << std::endl;
};
