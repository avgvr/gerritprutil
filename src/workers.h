// Copyright (c) 2026, Alexey Gavrilov


#include "git.h"
#include "handlers.h"
#include "transceiver.h"


template<class Connector>
void worker(EchoHandler &h, Connector &conn)
{
    Transceiver shadow(conn);

    auto peer = shadow.peerLink();
    Transceiver peerTransceiver(peer);

    peerTransceiver.acceptRequest(peerTransceiver.receivePacket());
    peerTransceiver.closeConnection();
};

template<class Connector>
void worker(HttpHandler &h, Connector &conn)
{
    Transceiver shadow(conn);

    auto peer = shadow.peerLink();
    Transceiver peerTransceiver(peer);

    h.acceptRequest(peerTransceiver.receivePacket());
    std::vector<unsigned char> response = h.getResponse();
    peerTransceiver.sendPacket(response);

    if(!h.isBodyValid()) throw std::runtime_error("Body is not valid");

    std::string repopath = "/tmp/" + h.repository().second;
    std::vector<std::string> prrefspec = {"+refs/pull/*:refs/pull*"};
    git::Repository repo(repopath);
    repo.fetchRemote(h.repository().first, prrefspec);
    auto cmts = repo.listCommits(h.base(), h.head());
    for(auto &cmt : cmts) std::cout << cmt->summary() << std::endl;

    // Fetch msg-hook from gerrit into above repository if it is not exists
    // Create external gerrit user imported from GitHub if it is not exists
    // Verify commits email
};
