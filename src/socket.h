// Copyright (c) 2026, Alexey Gavrilov


#pragma once


#include <type_traits>

#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define null 0

namespace sock
{

enum class socktype
{
    stream = SOCK_STREAM,
    datagram = SOCK_DGRAM
};

enum class domain
{
    localhost = AF_UNIX,
    inet = AF_INET,
    inet6 = AF_INET6
};

template <socktype T, domain D>
class GenericSocket
{
protected:
    static const socktype type = T;
    static const domain dom = D;

    using fd_t = int;

    fd_t fd;

    class ActiveEndpoint {public: virtual void connect() = 0;};
    class PassiveEndpoint {public: virtual void listen() = 0;};

public:
    GenericSocket() : fd(socket(static_cast<int>(D), static_cast<int>(T), 0)) {};
    GenericSocket(fd_t s) : fd(s) {};
    ~GenericSocket() {close(fd);};

    using AddrType =
    std::conditional_t
    <
        (static_cast<int>(D) == AF_UNIX),
        struct sockaddr_un,
        std::conditional_t
        <
            (static_cast<int>(D) == AF_INET),
            struct sockaddr_in,
            struct sockaddr_in6
        >
    >;

    void bind(const AddrType &addr)
    {
        if(
            bind(
                this->fd,
                reinterpret_cast<const sockaddr *>(&addr),
                sizeof(AddrType)
            ) == -1
        )
        {}; // Throw exception
    };

    using Active = ActiveEndpoint;
    using Passive = PassiveEndpoint;
};

template <socktype Type, domain Dom>
class ActiveDedicatedSocket : public GenericSocket<Type, Dom>, GenericSocket<Type, Dom>::Active
{
public:
    using Socket = GenericSocket<Type, Dom>;

    ActiveDedicatedSocket() : Socket() {};
    ActiveDedicatedSocket(typename Socket::fd_t s) : Socket(s) {};
    void connect() override final {};
};

template <socktype Type, domain Dom>
class PassiveDedicatedSocket : public GenericSocket<Type, Dom>, GenericSocket<Type, Dom>::Passive
{
public:
    using Socket = GenericSocket<Type, Dom>;

    PassiveDedicatedSocket() : GenericSocket<Type, Dom>(){};
    PassiveDedicatedSocket(typename Socket::fd_t s) : Socket(s) {};
    void listen() override final {};
};

};
