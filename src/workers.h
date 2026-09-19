// Copyright (c) 2026, Alexey Gavrilov


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
};
