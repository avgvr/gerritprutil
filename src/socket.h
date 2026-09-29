// Copyright (c) 2026, Alexey Gavrilov


#pragma once

#include <system_error>
#include <functional>
#include <thread>
#include <memory>

#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

const int null = 0;

namespace sock
{

using SslPointer = std::unique_ptr<SSL, decltype(&SSL_free)>;
using SslCtxPointer = std::shared_ptr<SSL_CTX>;

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
    explicit GenericSocket(const fd_t s) : fd(s) {};
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
        explicit Address(Type &a) : addr(a) {};

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
            addr.sin_port = htons(port);
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
            addr.sin_port = htons(port);
        };

        void setPlacement(const std::string &in6addr, const in_port_t port,
                            const uint32_t flowinfo)
        {
            static_assert(std::is_same_v<AddrType, struct sockaddr_in>);

            addr.sin6_family = family;
            addr.sin6_port = htons(port);
            addr.sin6_flowinfo = flowinfo;
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
            addr.sin6_port = htons(port);
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
        virtual void write(const std::vector<unsigned char> &data) = 0;
    };

    class PassiveEndpoint
    {
    public:
        virtual void listen(int backlog) = 0;

        struct client
        {
            typename GenericSocket<T, D>::Address addr;
            fd_t sock;
        };

        virtual client accept() = 0;

        virtual std::vector<unsigned char> read(const size_t n) = 0;
    };

public:
    using Active = ActiveEndpoint;
    using Passive = PassiveEndpoint;
};

template <socktype Type, domain Dom>
class PassiveDedicatedSocket;

template <socktype Type, domain Dom>
class ActiveDedicatedSocket : public GenericSocket<Type, Dom>, GenericSocket<Type, Dom>::Active
{
public:
    using Socket = GenericSocket<Type, Dom>;

    ActiveDedicatedSocket() : Socket() {};
    explicit ActiveDedicatedSocket(const fd_t s) : Socket(s) {};

    void connect(const typename GenericSocket<Type, Dom>::Address &addr) override
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

    void write(const std::vector<unsigned char> &data) override
    {
        ssize_t res = ::send(this->fd, data.data(), data.size(), null);

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
class EncryptedActiveSocket final : public ActiveDedicatedSocket<Type, Dom>
{
private:
    SslPointer ssl;
    SslCtxPointer ctx;

    void initializeSSL()
    {
        if(ctx == nullptr) return;

        SSL_CTX_set_verify(ctx.get(), SSL_VERIFY_PEER, nullptr);

        if(!SSL_CTX_set_default_verify_paths(ctx.get()))
        {
            ctx.reset();
            return;
        }
        if(!SSL_CTX_set_min_proto_version(ctx.get(), TLS1_2_VERSION))
        {
            ctx.reset();
            return;
        }

        ssl.reset(SSL_new(ctx.get()));
        if(ssl == nullptr)
        {
            ctx.reset();
        }
    };

public:
    EncryptedActiveSocket()
        : ssl(nullptr, &SSL_free),
        ctx(SSL_CTX_new(TLS_client_method()), &SSL_CTX_free)
    {initializeSSL();};

    explicit EncryptedActiveSocket(const fd_t s)
        : ActiveDedicatedSocket<Type, Dom>(s),
        ssl(nullptr, &SSL_free),
        ctx(SSL_CTX_new(TLS_client_method()), &SSL_CTX_free)
    {initializeSSL();};

    bool isEncryptionWorks() {return ssl and ctx;};

    void connect(const typename EncryptedActiveSocket::Address &addr) override final
    {
        std::array<char, 256> buff;
        ERR_error_string_n(ERR_peek_last_error(), buff.data(), buff.size());

        if(!(SSL_get_rbio(ssl.get()) and SSL_get_wbio(ssl.get())) and !SSL_set_fd(ssl.get(), this->fd))
            throw std::runtime_error("Failed to assign socket to a ssl session");

        if(isEncryptionWorks()())
            throw std::runtime_error(std::string(buff.data(), buff.size()));

        ActiveDedicatedSocket<Type, Dom>::connect(addr);

        int ret = SSL_connect(ssl.get());
        if(ret <= 0)
            throw std::runtime_error("Failed to create connection with error code: "
                                     + std::to_string(SSL_get_error(ssl.get(), ret)));
    };

    void write(const std::vector<unsigned char> &data) override final
    {
        std::array<char, 256> buff;
        ERR_error_string_n(ERR_peek_last_error(), buff.data(), buff.size());

        if(isEncryptionWorks()())
            throw std::runtime_error(std::string(buff.data(), buff.size()));

        int ret = SSL_write(ssl.get(), data.data(), data.size());
        if(ret <= 0) throw std::runtime_error("Failed to write data with error code: "
                                              + std::to_string(SSL_get_error(ssl.get(), ret)));
    };
};

template <socktype Type, domain Dom>
class PassiveDedicatedSocket : public GenericSocket<Type, Dom>, GenericSocket<Type, Dom>::Passive
{
public:
    using ActiveSocket = ActiveDedicatedSocket<Type, Dom>;
    using Socket = GenericSocket<Type, Dom>;

    PassiveDedicatedSocket() : Socket(){};
    explicit PassiveDedicatedSocket(const fd_t s) : Socket(s) {};
    PassiveDedicatedSocket(const fd_t d, PassiveDedicatedSocket &s) : Socket(d) {};

    void listen(int backlog) override final
    {
        int res = ::listen(this->fd, backlog);

        if(res == -1) throw std::system_error(errno, std::generic_category());
    };

    typename Socket::Passive::client accept() override final
    {
        using Sockaddr = typename GenericSocket<Type, Dom>::AddrType;

        struct sockaddr addr;
        socklen_t len;
        Sockaddr type;
        int fd = -1;

        for(;fd == -1;)
        {
            fd = ::accept(this->fd, &addr, &len);
            type = *reinterpret_cast<Sockaddr*>(&addr);

            if(fd == -1)
                if(!(errno == EAGAIN and errno == EWOULDBLOCK))
                    throw std::system_error(std::system_error(
                                errno,
                                std::generic_category())
                            );
        }

        return {typename Socket::Address(type), fd};
    };

    std::vector<unsigned char> read(const size_t n) override
    {
        std::vector<unsigned char> buffer(n + 1, null);
        size_t remain = n + 1;

        for(;remain > 0;)
        {
            int fetch =
                ::recv(
                        this->fd,
                        buffer.data() + (n + 1 - remain),
                        remain,
                        null
                    );

            if (fetch < 0)
            {
                if(!(errno == EAGAIN and errno == EWOULDBLOCK))
                    throw std::system_error(errno, std::generic_category());
                else return buffer;
            }
            else if (fetch == null) return buffer;
            else if (fetch > 0) remain -= fetch;
        }

        return buffer;
    };
};

template<socktype Type, domain Dom>
class EncryptedPassiveSocket : public PassiveDedicatedSocket<Type, Dom>
{
public:
    using ActiveSocket = EncryptedActiveSocket<Type, Dom>;

    constexpr static char DefaultChainFile[] = "cert.pem";
    constexpr static char DefaultPrivateFile[] = "key.pem";
    constexpr static unsigned char CacheID[] = "GitHub webhook server";
    struct requisiteFiles{std::string chain, privkey;};
private:
    SslPointer ssl;
    SslCtxPointer ctx;

    void initializeCtx(const requisiteFiles &files)
    {
        long opts = SSL_OP_IGNORE_UNEXPECTED_EOF
            | SSL_OP_NO_RENEGOTIATION
            | SSL_OP_CIPHER_SERVER_PREFERENCE;

        if(ctx == nullptr) return;

        if(!SSL_CTX_set_min_proto_version(ctx.get(), TLS1_2_VERSION))
        {
            ctx.reset();
        }

        SSL_CTX_set_options(ctx.get(), opts);
        if(SSL_CTX_use_certificate_chain_file(ctx.get(), files.chain.data()) <= 0)
        {
            ctx.reset();
            return;
        }

        if(SSL_CTX_use_PrivateKey_file(ctx.get(), files.privkey.data(), SSL_FILETYPE_PEM) <= 0)
        {
            ctx.reset();
            return;
        }

        if(SSL_CTX_set_session_id_context(ctx.get(), CacheID, sizeof(CacheID)) <= 0)
        {
            ctx.reset();
            return;
        }
        SSL_CTX_set_session_cache_mode(ctx.get(), SSL_SESS_CACHE_SERVER);
        SSL_CTX_sess_set_cache_size(ctx.get(), 16);
        SSL_CTX_set_timeout(ctx.get(), 3600);
        SSL_CTX_set_verify(ctx.get(), SSL_VERIFY_NONE, nullptr);
    };

public:
    EncryptedPassiveSocket(const requisiteFiles r =
                            {DefaultChainFile, DefaultPrivateFile})
        : ssl(nullptr, &SSL_free),
        ctx(SSL_CTX_new(TLS_server_method()), &SSL_CTX_free)
    {
        initializeCtx(r);
    };

    explicit EncryptedPassiveSocket(const fd_t s,
                                    const requisiteFiles r =
                                    {DefaultChainFile, DefaultPrivateFile})
        : PassiveDedicatedSocket<Type, Dom>(s),
        ssl(nullptr, &SSL_free),
        ctx(SSL_CTX_new(TLS_server_method()), &SSL_CTX_free)
    {
        initializeCtx(r);
    };

    EncryptedPassiveSocket(const fd_t d, EncryptedPassiveSocket &s)
        : PassiveDedicatedSocket<Type, Dom>(d),
        ssl(nullptr, &SSL_free),
        ctx(s.ctx)
    {};

    std::vector<unsigned char> read(const size_t n) override final
    {
        std::array<char, 256> buff;

        if(ctx == nullptr)
        {
            ERR_error_string_n(ERR_peek_error(), buff.data(), buff.size());
            throw std::runtime_error(buff.data());
        }

        if(ssl == nullptr)
        {
            ssl.reset(SSL_new(ctx.get()));
            if(ssl == nullptr)
            {
                ERR_error_string_n(ERR_get_error(), buff.data(), buff.size());
                throw std::runtime_error(buff.data());
            }

            if(!SSL_set_fd(ssl.get(), this->fd))
            {
                ERR_error_string_n(ERR_get_error(), buff.data(), buff.size());
                throw std::runtime_error(buff.data());
            }
        }

        std::vector<unsigned char> packet(n);

        int ret = SSL_read(ssl.get(), packet.data(), packet.size());
        if(ret <= 0)
        {
            char buff[256];
            int err = SSL_get_error(ssl.get(), ret);
            ERR_error_string_n(err, buff, 256);
            throw std::runtime_error("Failed to read data with error code: "
                                            + std::to_string(err)
                                            + ", " + buff);
        }

        return packet;
    };
};

template<socktype Type, domain Dom, class Sock = PassiveDedicatedSocket<Type, Dom>>
class SocketConnector final : public Sock
{
private:
    size_t backlog;
public:
    template<class... Ts>
    explicit SocketConnector(Ts... ps)
        : Sock(ps...), backlog(std::thread::hardware_concurrency())
    {};
    explicit SocketConnector(size_t bg) : backlog(bg), Sock() {};

    bool isClientValid()
    {
        try{
            int socktype = this->template getSocketOption<int>(Protocol::api, SocketOption::type);
        }
        catch(std::system_error &err)
        {
            return false;
        }

        return true;
    };

    void link() {};

    GenericSocket<socktype::stream, Dom>& getConnection() {return *this;}

    size_t getBacklog() {return this->backlog;};
};

template<domain Dom, class Sock>
class SocketConnector<socktype::stream, Dom, Sock> final
    : public Sock
{
private:
    std::unique_ptr<Sock> client;
    size_t backlog;
public:
    SocketConnector() : backlog(std::thread::hardware_concurrency()), Sock() {};
    explicit SocketConnector(size_t bg) : backlog(bg) {};
    template<class ...Ts>
    explicit SocketConnector(Ts... ps) : Sock(ps...) {};

    bool isClientValid()
    {
        int socktype;
        try{
            if(client)
                client->template getSocketOption<int>(
                    Protocol::api,
                    SocketOption::type
                );
            else return false;
        }
        catch(std::system_error &err)
        {
            return false;
        }

        return true;
    };

    void link()
    {
        this->client.reset(nullptr);
        this->client = std::make_unique<Sock>(this->accept().sock, dynamic_cast<Sock&>(*this));
    };

    GenericSocket<socktype::stream, Dom>& getConnection() {return *client;}

    size_t getBacklog() {return this->backlog;};

    void bind(const typename Sock::Address &addr)
    {
        Sock::bind(addr);
        this->listen(backlog);
    };
};

};
