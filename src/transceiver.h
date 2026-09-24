// Copyright (c) 2026, Alexey Gavrilov


#pragma once

#include "socket.h"

#include <thread>


template<sock::socktype Type, sock::domain Dom>
class Transceiver
{
private:
    sock::SocketConnector<Type, Dom> &scon;
public:
    constexpr static size_t MaxPackageSize = 1024 * 1024;

    Transceiver() = delete;
    Transceiver(sock::PassiveDedicatedSocket<Type, Dom> &s, const int bg)
        : scon(s, bg) {};
    Transceiver(sock::SocketConnector<Type, Dom> &sc) : scon(sc) {};

    std::vector<unsigned char> receivePacket()
    {
        if(!scon.isClientValid()) scon.link();

        sock::GenericSocket<Type, Dom> &sock = scon.getConnection();
        sock::PassiveDedicatedSocket<Type, Dom> &rsock =
            static_cast<sock::PassiveDedicatedSocket<Type, Dom>&>(sock);
        std::vector<unsigned char> buff = rsock.read(MaxPackageSize);

        if(buff.back() != null)
            throw std::runtime_error(
                "Maximum package size is reached - "
                + std::to_string(MaxPackageSize)
            );

        return buff;
    };

    void closeConnection()
    {
        scon.getConnection().~GenericSocket<Type, Dom>();
    }

    void sendPacket(std::vector<unsigned char> &content)
    {
        sock::GenericSocket<Type, Dom> &sock = scon.getConnection();
        sock::ActiveDedicatedSocket<Type, Dom> &wsock =
            static_cast<sock::ActiveDedicatedSocket<Type, Dom>&>(sock);

        wsock.write(content);
    };
};
