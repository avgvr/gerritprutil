// Copyright (c) 2026, Alexey Gavrilov


#pragma once

// #include <string>
#include <system_error>
// #include <type_traits>
// #include <cerrno>
// #include <stdexcept>
// #include <cstddef>
#include <vector>
#include <functional>
#include <thread>

#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

const int null = 0;


namespace sock
{

using fd_t = int;

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

    fd_t fd;

public:
    GenericSocket() : fd(socket(static_cast<int>(D), static_cast<int>(T), 0)) {};
    GenericSocket(const fd_t s) : fd(s) {};
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
        using Type = AddrType;

        Address() : addr({}) {};
        Address(Type &a) : addr(a) {};

        const sockaddr getSockaddr() const
        {
            return *reinterpret_cast<const sockaddr*>(&addr);
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
        sockaddr sockaddr = addr.getSockaddr();
        if(
            ::bind(
                this->fd,
                &sockaddr,
                sizeof(AddrType)
            ) == -1
        )
        {
            throw std::system_error(errno, std::generic_category());
        };
    };

    template<typename SockOptType>
    void setSocketOption(const Protocol lvl, const SocketOption opt, const SockOptType &val)
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
    SockOptType getSocketOption(const Protocol lvl, const SocketOption opt)
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

    class ActiveEndpoint
    {
    public:
        virtual void connect(const typename GenericSocket<T, D>::Address &addr) = 0;
        virtual void write(const std::vector<std::byte> &data) = 0;
    };

    class PassiveEndpoint
    {
    public:
        virtual void listen(int backlog) = 0;

        struct client
        {
            typename GenericSocket<T, D>::Address addr;
            fd_t socket;
        };

        virtual client accept() = 0;

        virtual std::vector<unsigned char> read(const size_t n) = 0;
    };

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

    ActiveDedicatedSocket(const fd_t s) : Socket(s) {};

    void connect(const typename GenericSocket<Type, Dom>::Address &addr) override final
    {
        size_t socklen;
        sockaddr sockaddr = addr.getSockaddr();
        int res =
            ::connect(
                this->fd,
                &sockaddr,
                sizeof(typename GenericSocket<Type, Dom>::Address::Type)
            );

        if(res < 0) throw std::system_error(errno, std::generic_category());
    };

    void write(const std::vector<std::byte> &data) override final
    {
        ssize_t res = ::write(this->fd, data.data(), data.size());

        if(res < 0) throw std::system_error(errno, std::generic_category());
        else if(res != data.size())
            throw std::runtime_error(
                "Is not whole write to socket - "
                + std::to_string(res)
                + "/"
                + std::to_string(data.size())
            );
    };
};

template <socktype Type, domain Dom>
class PassiveDedicatedSocket : public GenericSocket<Type, Dom>, GenericSocket<Type, Dom>::Passive
{
public:
    using Socket = GenericSocket<Type, Dom>;

    PassiveDedicatedSocket() : Socket(){};
    PassiveDedicatedSocket(const fd_t s) : Socket(s) {};

    void listen(int backlog) override final
    {
        int res = ::listen(this->fd, backlog);

        if(res == -1) throw std::system_error(errno, std::generic_category());
    };

    typename Socket::Passive::client accept() override final
    {
        struct sockaddr addr;
        socklen_t len;
        int socket = ::accept(this->fd, &addr, &len);

        using Sockaddr = typename GenericSocket<Type, Dom>::AddrType; 
        Sockaddr type = *reinterpret_cast<Sockaddr*>(&addr);

        if(socket == -1)
            throw std::system_error(std::system_error(errno, std::generic_category()));

        return {typename Socket::Address(type), socket};
    };

    std::vector<unsigned char> read(const size_t n) override final
    {
        std::vector<unsigned char> buffer(n + 1, null);
        size_t remain = n + 1;

        for(;remain > 0;)
        {
            int fetch = ::read(this->fd, buffer.data() + (n - remain), remain);

            if (fetch < 0) throw std::system_error(errno, std::generic_category());
            else if (fetch == null) return buffer;
            else if (fetch > 0) remain -= fetch;
        }

        return buffer;
    };
};

template<socktype Type, domain Dom>
class SocketConnector final : public PassiveDedicatedSocket<Type, Dom>
{
private:
    size_t backlog;
public:
    using PassiveDedicatedSocket =
        PassiveDedicatedSocket<Type, Dom>;

    SocketConnector() : backlog(std::thread::hardware_concurrency()) {};
    SocketConnector(size_t bg) : backlog(bg) {};

    void link() {};

    std::function<std::vector<unsigned char>(const size_t)>
    getReadFunction() {return this->read;};

    bool validness() {return true;}

    size_t getBacklog() {return this->backlog;};
};

template<domain Dom>
class SocketConnector<socktype::stream, Dom> final
    : public PassiveDedicatedSocket<socktype::stream, Dom>
{
private:
    PassiveDedicatedSocket<socktype::stream, Dom> client;
    size_t backlog;
public:
    using PassiveDedicatedSocket =
        PassiveDedicatedSocket<socktype::stream, Dom>;

    SocketConnector() :backlog(std::thread::hardware_concurrency()) {};
    SocketConnector(size_t bg) : backlog(bg) {};

    void link()
    {
        this->client =
            PassiveDedicatedSocket(this->accept().socket);
    };

    std::function<std::vector<unsigned char>(const size_t)>
    getReadFunction()
    {
        return
            std::bind(
                &PassiveDedicatedSocket::read,
                &this->client,
                std::placeholders::_1
            );
    };

    size_t getBacklog() {return this->backlog;};

    void bind(const typename PassiveDedicatedSocket::Address &addr)
    {
        PassiveDedicatedSocket::bind(addr);
        this->listen(backlog);
    };
};

};
