// Copyright (c) 2026, Alexey Gavrilov


#pragma once

#include "socket.h"

#include <thread>

template<sock::socktype Type, sock::domain Dom, class S>
class Transceiver
{
private:
    sock::SocketConnector<Type, Dom, S> &scon;
public:
    constexpr static size_t MaxPackageSize = 1024 * 1024;

    Transceiver() = delete;
    Transceiver(sock::SocketConnector<Type, Dom, S> &s, const int bg)
        : scon(s, bg) {};
    explicit Transceiver(sock::SocketConnector<Type, Dom, S> &sc) : scon(sc) {};

    std::vector<unsigned char> receivePacket()
    {
        if(!scon.isClientValid()) scon.link();

        sock::GenericSocket<Type, Dom> &sock = scon.getConnection();
        S &rsock =
            static_cast<S&>(sock);
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
        typename S::ActiveSocket &wsock =
            static_cast<typename S::ActiveSocket&>(sock);

        wsock.write(content);
    };
};
