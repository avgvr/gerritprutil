// Copyright (c) 2026, Alexey Gavrilov


#pragma once

#include "socket.h"

#include <thread>


template<sock::socktype Type, sock::domain Dom>
class Receiver
{
private:
    sock::SocketConnector<Type, Dom> &scon;
public:
    constexpr static size_t MaxPackageSize = 1024 * 1024;

    Receiver() = delete;
    Receiver(sock::PassiveDedicatedSocket<Type, Dom> &s, const int bg)
        : scon(s, bg) {};
    Receiver(sock::SocketConnector<Type, Dom> &sc) : scon(sc) {};

    std::vector<unsigned char> receivePackage()
    {
        scon.link();
        std::vector<unsigned char> buff = scon.getReadFunction()(MaxPackageSize);

        if(buff.back() != null)
            throw std::runtime_error(
                "Maximum package size is reached - "
                + std::to_string(MaxPackageSize)
            );

        return buff;
    };
};
