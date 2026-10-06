// Copyright (c) 2026, Alexey Gavrilov


#pragma once

#include <system_error>
#include <functional>
#include <thread>
#include <memory>
#include <optional>

#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <openssl/ssl.h>
#include <openssl/err.h>

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

class BasicEncryption
{
public:
    using SslPointer = std::unique_ptr<SSL, decltype(&SSL_free)>;
    using SslCtxPointer = std::shared_ptr<SSL_CTX>;
protected:
    SslPointer ssl;
    SslCtxPointer ctx;

public:
    BasicEncryption()
        : ssl(nullptr, &SSL_free),
        ctx(nullptr, &SSL_CTX_free)
    {};

    BasicEncryption(SSL_CTX *c)
        : ssl(nullptr, &SSL_free),
        ctx(c, &SSL_CTX_free)
    {};

    bool createNewSslSession()
    {
        ssl.reset(SSL_new(this->ctx.get()));
        if(ssl == nullptr) return false;
        else return true;
    };

    bool setSockToSslSession(fd_t d)
    {
        if(ssl == nullptr) return false;
        if(SSL_set_fd(ssl.get(), d)) return true;
        else return false;
    };

    bool isEncryptionWorks() {return this->ssl and this->ctx;};
};

template <socktype Type, domain Dom>
class PassiveDedicatedSocket;

template <socktype Type, domain Dom>
class ActiveDedicatedSocket : virtual public GenericSocket<Type, Dom>, GenericSocket<Type, Dom>::Active
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
class EncryptedActiveSocket : public ActiveDedicatedSocket<Type, Dom>,
                                virtual public BasicEncryption
{
protected:
    bool configureDefaultCtx()
    {
        if(this->ctx == nullptr) return false;

        SSL_CTX_set_verify(this->ctx.get(), SSL_VERIFY_PEER, nullptr);

        if(!SSL_CTX_set_default_verify_paths(this->ctx.get()))
        {
            this->ctx.reset();
            return false;
        }
        if(!SSL_CTX_set_min_proto_version(this->ctx.get(), TLS1_2_VERSION))
        {
            this->ctx.reset();
            return false;
        }
        return true;
    };

private:
    decltype(&EncryptedActiveSocket::configureDefaultCtx) ctxConfiguringCb = &EncryptedActiveSocket::configureDefaultCtx;;
public:
    EncryptedActiveSocket()
        : ActiveDedicatedSocket<Type, Dom>(),
        BasicEncryption(SSL_CTX_new(TLS_client_method()))
    {};

    explicit EncryptedActiveSocket(const fd_t s)
        : ActiveDedicatedSocket<Type, Dom>(s),
        BasicEncryption(SSL_CTX_new(TLS_client_method()))
    {};

    void connect(const typename GenericSocket<Type, Dom>::Address &addr) override
    {
        std::array<char, 256> buff;

        if(this->ctx == nullptr)
        {
            this->ctx.reset(SSL_CTX_new(TLS_client_method()), &SSL_CTX_free);
            if(this->ctx == nullptr) 
            {
                ERR_error_string_n(ERR_get_error(), buff.data(), buff.size());
                throw std::runtime_error("Failed to create SSL context"
                                            + std::string(buff.data()));
            }
        }

        if(ctxConfiguringCb and !(this->*ctxConfiguringCb)())
        {
            ERR_error_string_n(ERR_get_error(), buff.data(), buff.size());
            throw std::runtime_error("Falied to create a SSL context"
                                        + std::string(buff.data()));
        }
        else if(ctxConfiguringCb) ctxConfiguringCb = nullptr;

        if(this->ssl == nullptr and !this->createNewSslSession())
        {
            ERR_error_string_n(ERR_get_error(), buff.data(), buff.size());
            throw std::runtime_error("Failed to create a SSL session"
                                        + std::string(buff.data()));
        }

        if(!this->setSockToSslSession(this->fd))
        {
            ERR_error_string_n(ERR_get_error(), buff.data(), buff.size());
            throw std::runtime_error("Failed to assign socket to a SSL session"
                                        + std::string(buff.data()));
        }

        ActiveDedicatedSocket<Type, Dom>::connect(addr);

        int ret = SSL_connect(this->ssl.get());
        if(ret <= 0)
            throw std::runtime_error("Failed to create connection with error code: "
                                        + std::to_string(
                                        SSL_get_error(this->ssl.get(), ret)
                                        )
                                    );
    };

    void write(const std::vector<unsigned char> &data) override final
    {
        if(!this->isEncryptionWorks())
            throw std::runtime_error("Encryption is not works");

        int ret = SSL_write(this->ssl.get(), data.data(), data.size());
        if(ret <= 0) throw std::runtime_error("Failed to write data with error code: "
                                              + std::to_string(SSL_get_error(this->ssl.get(), ret)));
    };
};

template <socktype Type, domain Dom>
class PassiveDedicatedSocket : virtual public GenericSocket<Type, Dom>, GenericSocket<Type, Dom>::Passive
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

    typename Socket::Passive::client accept() override
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
        size_t remain = n + 1;
        std::vector<unsigned char> buffer(remain, null);

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
class EncryptedPassiveSocket : public PassiveDedicatedSocket<Type, Dom>,
                                virtual public BasicEncryption
{
public:
    constexpr static char DefaultChainFile[] = "cert.pem";
    constexpr static char DefaultPrivateFile[] = "key.pem";
    constexpr static unsigned char CacheID[] = "GitHub webhook server";
    struct requisiteFiles{std::string chain, privkey;};

protected:
    bool configureDefaultCtx(const requisiteFiles files =
            {DefaultChainFile, DefaultPrivateFile})
    {
        if(this->ctx == nullptr) return false;

        constexpr long opts = SSL_OP_IGNORE_UNEXPECTED_EOF
            | SSL_OP_NO_RENEGOTIATION
            | SSL_OP_CIPHER_SERVER_PREFERENCE;

        if(!SSL_CTX_set_min_proto_version(this->ctx.get(), TLS1_2_VERSION))
        {
            this->ctx.reset();
        }

        SSL_CTX_set_options(this->ctx.get(), opts);
        if(SSL_CTX_use_certificate_chain_file(this->ctx.get(), files.chain.data()) <= 0)
        {
            this->ctx.reset();
            return false;
        }

        if(SSL_CTX_use_PrivateKey_file(this->ctx.get(), files.privkey.data(), SSL_FILETYPE_PEM) <= 0)
        {
            this->ctx.reset();
            return false;
        }

        if(SSL_CTX_set_session_id_context(this->ctx.get(), CacheID, sizeof(CacheID)) <= 0)
        {
            this->ctx.reset();
            return false;
        }
        SSL_CTX_set_session_cache_mode(this->ctx.get(), SSL_SESS_CACHE_SERVER);
        SSL_CTX_sess_set_cache_size(this->ctx.get(), 16);
        SSL_CTX_set_timeout(this->ctx.get(), 3600);
        SSL_CTX_set_verify(this->ctx.get(), SSL_VERIFY_NONE, nullptr);

        return true;
    };

private:
    requisiteFiles reqfiles{DefaultChainFile, DefaultPrivateFile};
    decltype(&EncryptedPassiveSocket::configureDefaultCtx) ctxConfiguringCb = &EncryptedPassiveSocket::configureDefaultCtx;

public:
    EncryptedPassiveSocket()
        :PassiveDedicatedSocket<Type, Dom>(),
        BasicEncryption(SSL_CTX_new(TLS_server_method()))
    {};

    explicit EncryptedPassiveSocket(const requisiteFiles r)
        :PassiveDedicatedSocket<Type, Dom>(),
        BasicEncryption(SSL_CTX_new(TLS_server_method())),
        reqfiles{r.chain, r.privkey}
    {};

    explicit EncryptedPassiveSocket(const fd_t s,
                                    const requisiteFiles r =
                                    {DefaultChainFile, DefaultPrivateFile})
        : PassiveDedicatedSocket<Type, Dom>(s),
        BasicEncryption(SSL_CTX_new(TLS_server_method())),
        requisiteFiles{r.chain, r.privkey}
    {};

    EncryptedPassiveSocket(const fd_t d, EncryptedPassiveSocket &s)
        : PassiveDedicatedSocket<Type, Dom>(d),
        BasicEncryption(s.ctx),
        reqfiles{s.reqfiles.chain, s.reqfiles.privkey},
        ctxConfiguringCb(s.ctxConfiguringCb)
    {};

    typename GenericSocket<Type, Dom>::Passive::client accept() override final
    {
        std::array<char, 256> buff;

        if(this->ctx == nullptr)
        {
            this->ctx.reset(SSL_CTX_new(TLS_client_method()), &SSL_CTX_free);
            if(this->ctx == nullptr) 
            {
                ERR_error_string_n(ERR_get_error(), buff.data(), buff.size());
                throw std::runtime_error("Failed to create SSL context"
                                            + std::string(buff.data()));
            }
        }

        if(ctxConfiguringCb and !(this->*ctxConfiguringCb)(reqfiles))
        {
            ERR_error_string_n(ERR_get_error(), buff.data(), buff.size());
            throw std::runtime_error("Falied to create a SSL context" + std::string(buff.data()));
        }
        else if(ctxConfiguringCb) ctxConfiguringCb = nullptr;

        return PassiveDedicatedSocket<Type, Dom>::accept();
    };

    std::vector<unsigned char> read(const size_t n) override final
    {
        if(!this->isEncryptionWorks())
            throw std::runtime_error("Encryption is not works");

        std::array<char, 256> buff;

        size_t remain = n + 1;
        std::vector<unsigned char> packet(remain, null);

        while(remain > 0)
        {
            size_t read;
            bool isReadSuccess = SSL_read_ex(
                                this->ssl.get(),
                                packet.data() + (n + 1 - remain),
                                remain,
                                &read
                            );
            if(isReadSuccess)
            {
                remain -= read;
            }
            else
            {
                size_t pending = SSL_has_pending(this->ssl.get()) ?
                                    SSL_pending(this->ssl.get()) : 0;
                if(!pending) break;

                int err = SSL_get_error(this->ssl.get(), isReadSuccess);
                // Error is retryable
                if(err == SSL_ERROR_WANT_READ) continue;
                // Client cancel connection
                else if(err == SSL_ERROR_ZERO_RETURN) break;
                // Error is non-retryable
                else
                {
                    char buff[256];
                    // ERR_error_string_n(err, buff, 256);
                    throw std::runtime_error("Failed to read data with error code: "
                                                    + std::to_string(err)
                                                    + ", " + buff);

                }
            }
        }

        return packet;
    };
};

template<socktype Sock, domain Dom>
class SocketConnector : public ActiveDedicatedSocket<Sock, Dom>,
                        public PassiveDedicatedSocket<Sock, Dom>
{
public:
    using Socket = GenericSocket<Sock, Dom>;
    using Address = typename Socket::Address;
private:
    std::optional<Address> a;
    using PassiveDedicatedSocket<Sock, Dom>::accept;
public:
    SocketConnector() : Socket() {};
    SocketConnector(fd_t d) : Socket(d) {};

    void connect()
    {
        if(a.has_value()) ActiveDedicatedSocket<Sock, Dom>::connect(a);
        else throw std::runtime_error("Unable perform reconnection");
    };

    void connect(const Address &addr) override final
    {
        this->a = addr;
        ActiveDedicatedSocket<Sock, Dom>::connect(addr);
    };

    SocketConnector peerAccept()
    {
        return SocketConnector(PassiveDedicatedSocket<Sock, Dom>::accept().sock);
    };
};

template<domain Dom>
class SocketConnector<socktype::datagram, Dom> {};

template<socktype Sock, domain Dom>
class EncryptedSocketConnector : virtual public EncryptedActiveSocket<Sock, Dom>,
                                virtual public EncryptedPassiveSocket<Sock, Dom>
{
public:
    using Socket = GenericSocket<Sock, Dom>;
    using Address = typename Socket::Address;
private:
    std::optional<Address> a;
    using EncryptedPassiveSocket<Sock, Dom>::accept;
public:
    EncryptedSocketConnector() : Socket(), BasicEncryption() {};
    EncryptedSocketConnector(fd_t d) : Socket(d), BasicEncryption() {};

    void connect()
    {
        if(a.has_value()) EncryptedActiveSocket<Sock, Dom>::connect(a);
        else throw std::runtime_error("Unable perform reconnection");
    };

    void connect(const typename GenericSocket<Sock, Dom>::Address &addr) override final
    {
        this->a = addr;

        EncryptedActiveSocket<Sock, Dom>::connect(addr);
    };

    EncryptedSocketConnector peerAccept()
    {
        std::array<char, 256> buff;
        auto &res = SocketConnector(EncryptedPassiveSocket<Sock, Dom>::accept().sock, *this);

        if(!res.createNewSslSession())
        {
            ERR_error_string_n(ERR_get_error(), buff.data(), buff.size());
            throw std::runtime_error("Failed to create a SSL session" + std::string(buff.data()));
        }

        if(!this->setFdToSslSession(this->fd))
        {
            ERR_error_string_n(ERR_get_error(), buff.data(), buff.size());
            throw std::runtime_error("Failed to create a SSL session" + std::string(buff.data()));
        }

        return res;
    };
};

template<domain Dom>
class EncryptedSocketConnector<socktype::datagram, Dom> {};

};
