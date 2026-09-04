// Copyright (c) 2026, Alexey Gavrilov


#pragma once


#include <string>
#include <type_traits>
#include <system_error>
#include <cerrno>
#include <stdexcept>

#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

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

enum class SocketOption
{
    acceptConn =        SO_ACCEPTCONN,
    broadcast =         SO_BROADCAST,
    debug =             SO_DEBUG,
    dontRoute =         SO_DONTROUTE,
    error =             SO_ERROR,
    keepAlive =         SO_KEEPALIVE,
    linger =            SO_LINGER,
    oobinline =         SO_OOBINLINE,
    rcvbuf =            SO_RCVBUF,
    rcvlowat =          SO_RCVLOWAT,
    rcvtimeo =          SO_RCVTIMEO,
    reuseaddr =         SO_REUSEADDR,
    sndBuf =            SO_SNDBUF,
    sndlowat =          SO_SNDLOWAT,
    sndtimeo =          SO_SNDTIMEO,
    type =              SO_TYPE
};

enum class Protocol
{
    api =               SOL_SOCKET,
    tcp =               IPPROTO_TCP,
    ip =                IPPROTO_IP,
    ipv6 =              IPPROTO_IPV6,
    udp =               IPPROTO_UDP
};

template <socktype T, domain D>
class GenericSocket
{
protected:
    static const socktype type = T;
    static const domain dom = D;
    static const sa_family_t family = static_cast<sa_family_t>(dom);

    using fd_t = int;

    fd_t fd;

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

    static_assert(
                    static_cast<int>(D) == AF_UNIX or
                    static_cast<int>(D) == AF_INET or
                    static_cast<int>(D) == AF_INET6
    );

    class Address
    {
    private:
        AddrType addr;
    public:
        Address() : addr({}) {};
        const sockaddr* getSockaddr() const
        {
            return reinterpret_cast<const sockaddr*>(&addr);
        };

        void setPlacement(const std::string &filepath)
        {
            static_assert(std::is_same_v<AddrType, struct sockaddr_un>);

            addr.sun_family = static_cast<sa_family_t>(dom);
            filepath.copy(&addr.sun_path, sizeof(addr.sun_path) - 1);
        };

        void setPlacement(const std::string &inaddr, const in_port_t port)
        {
            static_assert(std::is_same_v<AddrType, struct sockaddr_in>);

            addr.sin_family = family;
            int r = inet_pton(family, inaddr.c_str(), &addr.sin_addr);

            if (r == 0)
            {
                throw std::runtime_error("Address is not in presentation format");
            }
            else if(r < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }
        };

        void setPlacement(const struct in_addr inaddr, const in_port_t port)
        {
            static_assert(std::is_same_v<AddrType, struct sockaddr_in>);

            addr.sin_family = family;
            addr.sin_addr = inaddr;
            addr.sin_port = port;
        };

        void setPlacement(const std::string &in6addr, const in_port_t port,
                            const uint32_t flowinfo)
        {
            static_assert(std::is_same_v<AddrType, struct sockaddr_in>);

            addr.sin6_family = family;
            int r = inet_pton(family, in6addr.c_str(), &addr.sin_addr);

            if (r == 0)
            {
                throw std::runtime_error("Address is not in presentation format");
            }
            else if(r < 0)
            {
                throw std::system_error(errno, std::generic_category());
            }
        };

        void setPlacement(const struct in6_addr &in6addr, const in_port_t port,
                            const uint32_t flowinfo)
        {
            static_assert(std::is_same_v<AddrType, struct sockaddr_in>);

            addr.sin6_family = family;
            addr.sin6_addr = in6addr;
            addr.sin6_port = port;
            addr.sin6_flowinfo = flowinfo;
        };
    };

    void bind(const Address &addr)
    {
        if(
            ::bind(
                this->fd,
                addr.getSockaddr(),
                sizeof(AddrType)
            ) == -1
        )
        {
            throw std::system_error(errno, std::generic_category());
        };
    };

    template<typename SockOptType>
    void setSocketOption(Protocol lvl, SocketOption opt, SockOptType &val)
    {
        int ret = setsockopt(
                    this->fd,
                    static_cast<int>(lvl),
                    static_cast<int>(opt),
                    std::addressof(val),
                    sizeof(val)
        );

        if(ret == -1) throw std::system_error(errno, std::generic_category());
    };

    template<typename SockOptType>
    SockOptType getSocketOption(Protocol lvl, SocketOption opt)
    {
        SockOptType optval;
        socklen_t optlen;

        int ret = getsockopt(
            this->fd,
            static_cast<int>(lvl),
            static_cast<int>(opt),
            &optval,
            &optlen
        );

        if(ret == -1) throw std::system_error(errno, std::generic_category());
        if(optlen != sizeof(optval))
        {
            std::string error = "Return type size error - "
                + std::to_string(sizeof(optval))
                +  ", expected size is "
                + std::to_string(optlen);

            throw std::runtime_error(error);
        }
        return optval;
    };

protected:

    class ActiveEndpoint {public: virtual void connect(typename GenericSocket<T, D>::Address addr) = 0;};
    class PassiveEndpoint {public: virtual void listen() = 0;};

public:
    using Active = ActiveEndpoint;
    using Passive = PassiveEndpoint;
};

template <socktype Type, domain Dom>
class ActiveDedicatedSocket final : public GenericSocket<Type, Dom>, GenericSocket<Type, Dom>::Active
{
public:
    using Socket = GenericSocket<Type, Dom>;

    ActiveDedicatedSocket() : Socket() {};
    ActiveDedicatedSocket(typename Socket::fd_t s) : Socket(s) {};
    void connect(typename GenericSocket<Type, Dom>::Address addr) override final {};
};

template <socktype Type, domain Dom>
class PassiveDedicatedSocket final : public GenericSocket<Type, Dom>, GenericSocket<Type, Dom>::Passive
{
public:
    using Socket = GenericSocket<Type, Dom>;

    PassiveDedicatedSocket() : GenericSocket<Type, Dom>(){};
    PassiveDedicatedSocket(typename Socket::fd_t s) : Socket(s) {};
    void listen() override final {};
};

};
