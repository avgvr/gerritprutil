// Copyright (c) 2026, Alexey Gavrilov


#pragma once

#include "socket.h"

#include <thread>

template<class Connector>
class Transceiver
{
private:
    Connector &scon;
public:
    constexpr static size_t MaxPacketSize = 1024 * 1024;

    Transceiver() = delete;
    explicit Transceiver(Connector &s) : scon(s) {};

    Connector peerLink() {return scon.peerAccept();};

    std::vector<unsigned char> receivePacket()
    {
        std::vector<unsigned char> buff = scon.read(MaxPacketSize);

        if(buff.back() != null and buff.size() == MaxPacketSize)
            throw std::runtime_error(
                "Maximum package size is reached - "
                + std::to_string(MaxPacketSize)
            );

        return buff;
    };

    void sendPacket(std::vector<unsigned char> &content)
    {
        scon.write(content);
    };
};
