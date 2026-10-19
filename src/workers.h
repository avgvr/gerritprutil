// Copyright (c) 2026, Alexey Gavrilov


#include "git.h"
#include "handlers.h"
#include "transceiver.h"
#include "parser.h"

#include <fstream>
#include <filesystem>
#include <charconv>

#include <jwt-cpp/jwt.h>

static std::string GERRITHOSTNAME = "";
static in_port_t GERRITPORT = 0;
static std::string GERRITUSER = "";
static std::string GERRITPASSWORD = "";
static std::string PUSHBRANCH = "main";
static std::string GITHUBCLIENTID = "";
static std::string GITHUBKEYPATH = "";

void githubapptokenid_cb(argument &o){GITHUBCLIENTID = o.value;};
void githubkeypath_cb(argument &o){GITHUBKEYPATH = o.value;};

void pushbranch_pr(argument &o) {PUSHBRANCH = o.value;};

void gerritaddress_cb(argument &o)
{
    size_t delimPosition = o.value.find_first_of(':');

    auto delimIt = delimPosition != std::string::npos ?
        o.value.begin() + delimPosition
        : o.value.end();

    std::string host(o.value.begin(), delimIt),
        port(
            delimIt != o.value.end() ?
                delimIt + 1
                : o.value.end(),
            o.value.end()
        );

    if(!(delimPosition == std::string::npos))
    {
        GERRITHOSTNAME = host;
        GERRITPORT = std::stoul(port);
    }
};
void gerritusername_cb(argument &o){GERRITUSER = o.value;};
void gerritpassword_cb(argument &o){GERRITPASSWORD = o.value;};

template<class Connector>
void worker(EchoHandler &h, Connector &conn)
{
    Transceiver shadow(conn);

    auto peer = shadow.peerLink();
    Transceiver peerTransceiver(peer);

    peerTransceiver.acceptRequest(peerTransceiver.receivePacket());
    peerTransceiver.closeConnection();
};

using EncryptedConnector =
    sock::EncryptedSocketConnector<sock::socktype::stream, sock::domain::inet>;

constexpr static int trueint = 1;
constexpr static struct timeval timeout = {2, 0};

std::string fetchGithubUserName(const std::string &username)
{
    const std::string contentTypeJson = "application/json";

    EncryptedConnector githubApiSocket;
    EncryptedConnector::Address githubApiAddr;
    githubApiAddr.setPlacement("api.github.com", 443);
    githubApiSocket.setSocketOption(sock::Protocol::api, sock::SocketOption::reuseaddr, trueint);
    githubApiSocket.setSocketOption(sock::Protocol::api, sock::SocketOption::rcvtimeo, timeout);
    githubApiSocket.connect(githubApiAddr);

    Transceiver githubApi(githubApiSocket);

    http::Http githubUserCredentials;
    http::Header &hdr = githubUserCredentials.getHttpHeader();
    hdr.setHeader("method", "GET");
    hdr.setHeader("path", "/users/" + username);
    hdr.setHeader("version", "HTTP/1.1");
    hdr.setHeader("Host", "api.github.com");
    hdr.setHeader("X-GitHub-Api-Version", "2026-03-10");
    hdr.setHeader("Accept", "application/vnd.github+json");
    hdr.setHeader("User-Agent", username);

    std::vector<unsigned char> httpraw = githubUserCredentials.dump();

    githubApi.sendPacket(httpraw);

    auto data = githubApi.receivePacket();
    githubUserCredentials << data;
    auto httpContent = githubUserCredentials.getHttpContent();

    if(httpContent.first.size() == 0)
    {
        throw std::runtime_error("Data are not present in HTTP body");
    }

    if(httpContent.second.find(contentTypeJson) == std::string::npos)
    {
        const std::string err = "Invalid content type - " +
            httpContent.second +
            ", expected - " +
            contentTypeJson;
        throw std::runtime_error(err);
    }

    unsigned httpCode = std::stol(githubUserCredentials.getHttpHeader().getValue("code"));
    if(httpCode != 200)
        throw std::runtime_error("HTTP code іs not OK - " + std::to_string(httpCode));

    nlohmann::json credentials = nlohmann::json::parse(
        std::string(
            httpContent.first.begin(),
            httpContent.first.end()
        )
    );

    return {credentials["name"]};
};

void fetchSshPublicKey(const std::string &username, const std::string &authorname, const unsigned githubUserId, const std::string &pubkeyFilepath)
{
    EncryptedConnector gerritSocket;
    EncryptedConnector::Address gerritAddr;
    gerritAddr.setPlacement(GERRITHOSTNAME, GERRITPORT);
    gerritSocket.setSocketOption(sock::Protocol::api, sock::SocketOption::reuseaddr, trueint);
    gerritSocket.setSocketOption(sock::Protocol::api, sock::SocketOption::rcvtimeo, timeout);
    gerritSocket.connect(gerritAddr);

    Transceiver transceiver(gerritSocket);

    http::Http httpPacket;
    auto &httpHeader = httpPacket.getHttpHeader();
    httpHeader.setHeader("method", "GET");
    httpHeader.setHeader("path", "/accounts/" + username);
    httpHeader.setHeader("version", "HTTP/1.1");
    httpHeader.setHeader("Host", GERRITHOSTNAME);

    auto rawbytes = httpPacket.dump();
    transceiver.sendPacket(rawbytes);
    rawbytes = transceiver.receivePacket();
    httpPacket << rawbytes;

    const std::string authString = GERRITUSER + ':' + GERRITPASSWORD;

    unsigned responseCode = std::stol(httpPacket.getHttpHeader().getValue("code"));
    if(responseCode == 404) // User not found
    {
        httpPacket.erase();
        httpHeader.setHeader("method", "PUT");
        httpHeader.setHeader("path", "/a/accounts/" + username);
        httpHeader.setHeader("version", "HTTP/1.1");
        httpHeader.setHeader("Host", GERRITHOSTNAME);
        httpHeader.addBasicAuthentication(authString);

        nlohmann::json gerritUser = nlohmann::json::object();
        gerritUser["username"] = username;
        gerritUser["name"] = authorname;
        gerritUser["ext_id"] = "oauth:github-oauth:" + std::to_string(githubUserId);

        auto gerritUserDetails = gerritUser.dump();
        httpHeader.setHeader("Content-Length", std::to_string(gerritUserDetails.length()));

        httpPacket.setHttpContent(
            {gerritUserDetails.begin(), gerritUserDetails.end()},
            "application/json"
        );

        rawbytes = httpPacket.dump();
        transceiver.sendPacket(rawbytes);
        rawbytes = transceiver.receivePacket();
        httpPacket << rawbytes;

        responseCode = std::stol(httpPacket.getHttpHeader().getValue("code"));
        if(responseCode != 200)
        {
            auto errorString = httpPacket.getHttpContent().first;
            throw std::runtime_error(std::string(errorString.begin(), errorString.end()));
        }
    }
    else if(responseCode != 200)
    {
        auto errorString = httpPacket.getHttpContent().first;
        throw std::runtime_error(std::string(errorString.begin(), errorString.end()));
    }

    // Obtain the user ssh key
    httpPacket.erase();
    httpHeader.setHeader("method", "GET");
    httpHeader.setHeader("path", "/a/accounts/" + username + "/sshkeys/1");
    httpHeader.setHeader("version", "HTTP/1.1");
    httpHeader.setHeader("Host", GERRITHOSTNAME);
    httpHeader.addBasicAuthentication(authString);

    rawbytes = httpPacket.dump();
    transceiver.sendPacket(rawbytes);
    rawbytes = transceiver.receivePacket();
    httpPacket << rawbytes;

    responseCode = std::stol(httpPacket.getHttpHeader().getValue("code"));
    if(responseCode != 200)
        throw std::runtime_error("SSH key is not set for the " + username + " user");

    constexpr static unsigned short PassthroughJsonPrefix = 4;
    auto userSshKey = httpPacket.getHttpContent().first;
    nlohmann::json keyjson =
        nlohmann::json::parse(
            std::string(
                userSshKey.begin() + PassthroughJsonPrefix,
                userSshKey.end()
            )
        );

    std::ofstream pubkey(pubkeyFilepath);
    if(pubkey.is_open())
    {
        std::string key = keyjson["ssh_public_key"];
        pubkey << key;
        pubkey.close();
    }
    else
        throw std::runtime_error("Unable open file ");
};

void fetchGitMsgHook(const std::string &repopath)
{
    const std::filesystem::path hookpath = repopath + "/.git/hooks/commit-msg";

    if(std::filesystem::exists(hookpath)) return;

    EncryptedConnector gerritSocket;
    EncryptedConnector::Address gerritAddr;
    gerritAddr.setPlacement(GERRITHOSTNAME, GERRITPORT);
    gerritSocket.setSocketOption(sock::Protocol::api, sock::SocketOption::reuseaddr, trueint);
    gerritSocket.setSocketOption(sock::Protocol::api, sock::SocketOption::rcvtimeo, timeout);
    gerritSocket.connect(gerritAddr);

    Transceiver transceiver(gerritSocket);

    http::Http httpPacket;
    auto &httpHeader = httpPacket.getHttpHeader();
    httpHeader.setHeader("method", "GET");
    httpHeader.setHeader("path", "/tools/hooks/commit-msg");
    httpHeader.setHeader("version", "HTTP/1.1");
    httpHeader.setHeader("Host", GERRITHOSTNAME);

    auto rawbytes = httpPacket.dump();
    transceiver.sendPacket(rawbytes);
    rawbytes = transceiver.receivePacket();
    httpPacket << rawbytes;

    auto commitMsgContent = httpPacket.getHttpContent().first;
    std::ofstream commitMsgFile(hookpath);
    if(commitMsgFile.is_open())
    {
        commitMsgFile << std::string(commitMsgContent.begin(), commitMsgContent.end());
        std::filesystem::permissions(hookpath, std::filesystem::perms::owner_exec);
    }
    else throw std::runtime_error("commit-msg hook fetch failed");
};

void githubLeaveMessage(
    const std::string &authToken,
    const size_t prid,
    const std::string &user,
    const std::string &repository,
    const std::string &msg
)
{
    const std::string contentTypeJson = "application/json";

    EncryptedConnector githubApiSocket;
    EncryptedConnector::Address githubApiAddr;
    githubApiAddr.setPlacement("api.github.com", 443);
    githubApiSocket.setSocketOption(sock::Protocol::api, sock::SocketOption::reuseaddr, trueint);
    githubApiSocket.setSocketOption(sock::Protocol::api, sock::SocketOption::rcvtimeo, timeout);
    githubApiSocket.connect(githubApiAddr);

    Transceiver githubApi(githubApiSocket);

    http::Http githubPrComment;
    http::Header &hdr = githubPrComment.getHttpHeader();
    hdr.setHeader("method", "POST");
    hdr.setHeader(
                    "path",
                    "/repos/" 
                    + user + '/'
                    + repository
                    + "/issues/"
                    + std::to_string(prid)
                    + "comments"
                );
    hdr.setHeader("version", "HTTP/1.1");
    hdr.setHeader("Host", "api.github.com");
    hdr.setHeader("X-GitHub-Api-Version", "2026-03-10");
    hdr.setHeader("Accept", "application/vnd.github+json");
    hdr.setHeader("User-Agent", user);
    hdr.addBearerAuthentication(authToken);

    nlohmann::json comment = nlohmann::json::object();
    comment["body"] = msg;

    auto commentDump = comment.dump();
    hdr.setHeader("Content-Length", std::to_string(commentDump.length()));

    githubPrComment.setHttpContent(
        {commentDump.begin(), commentDump.end()},
        "application/json"
    );


    std::vector<unsigned char> httpraw = githubPrComment.dump();

    githubApi.sendPacket(httpraw);

    auto data = githubApi.receivePacket();
    githubPrComment << data;
    auto httpContent = githubPrComment.getHttpContent();

    unsigned httpCode = std::stol(githubPrComment.getHttpHeader().getValue("code"));
    if(httpCode != 200)
        throw std::runtime_error("HTTP code іs not OK - " + std::to_string(httpCode));
};

template<class Connector>
void worker(HttpHandler &h, Connector &conn)
{
    Transceiver shadow(conn);
    std::string botMsg = "";

    while(true)
    {
        botMsg = "";

        auto peer = shadow.peerLink();
        Transceiver peerTransceiver(peer);

        h.acceptRequest(peerTransceiver.receivePacket());
        std::vector<unsigned char> response = h.getResponse();
        peerTransceiver.sendPacket(response);

        try
        {

            if(!h.isBodyValid()) throw std::runtime_error("Body is not valid");

            const std::filesystem::path repopath = "/tmp/" + h.repository().second;
            const std::filesystem::path pubkeypath = "/tmp/pubkey";

            const std::vector<std::string> prrefspec = {"+refs/pull/*:refs/pull*"};

            git::Repository repository(repopath);
            repository.fetchRemote(h.repository().first, prrefspec);

            if(GERRITHOSTNAME.empty() or GERRITPORT == 0)
                throw std::runtime_error("Gerrit address is not set");
            else if(GERRITUSER.empty()) throw std::runtime_error("Gerrit user is not set");
            else if(GERRITPASSWORD.empty()) throw std::runtime_error("Gerrit password is not set");

            // Fetch msg-hook from gerrit into above repository if it is not exists
            fetchGitMsgHook(repopath);

            // Obtain user credential from GitHub REST API
            std::string githubUserName = fetchGithubUserName(h.getUserLogin());

            // Create external gerrit user imported from the GitHub if it is not exists
            // Fetch SSH key to push git commits
            fetchSshPublicKey(h.getUserLogin(), githubUserName, h.getUserId(), pubkeypath);

            // Push commits to a Gerrit
            auto prcmts = repository.listCommits(h.base(), h.head());
            botMsg = "Hello, @" + h.getUserLogin() + ". Your PR was moved to a Gerrit Code Review.\n\n";
            const std::string indent = "    ";
            for(auto &cmt : prcmts)
            {
                std::string progress = repository.pushRemote(
                    GERRITUSER + '@' + GERRITHOSTNAME,
                    {},
                    {cmt->hash() + ":refs/for/" + PUSHBRANCH},
                    h.getUserLogin()
                );
                auto changeUrlPosBegin = progress.find_first_of("https://" + GERRITHOSTNAME + "/c/" + h.repository().second + "/+/");
                if(changeUrlPosBegin != std::string::npos)
                {
                    auto changeUrlPosEnd = progress.find_first_of('\n', changeUrlPosBegin);
                    if(changeUrlPosEnd != std::string::npos)
                    {
                        progress = indent + std::string(changeUrlPosBegin, changeUrlPosEnd + 1);
                    }
                    else progress = "";
                }
                else progress = "";

                botMsg += progress;
            }
            botMsg += "\n";

        }
        catch(std::runtime_error &runtimeError)
        {
            botMsg = "Failed to upload PR to a Gerrit";
            std::cerr << runtimeError.what() << std::endl;
        }
        catch(std::exception &otherError)
        {
            botMsg = "Hello, @" + h.getUserLogin() + ". Your PR cannot to move into Gerrit Code Review due internal error.\n";
            std::cerr << otherError.what() << std::endl;
        }
        catch(...)
        {
            botMsg = "Failed to upload PR to a Gerrit";
            std::cerr << "Unexpected internal error caught" << std::endl;
        }

        try
        {
            if(GITHUBCLIENTID.empty() or GITHUBKEYPATH.empty())
                throw std::runtime_error("GitHub App credentials is not set");

            // read GitHub private file
            std::ifstream keyfile(GITHUBKEYPATH);

            if(!keyfile.is_open())
                throw std::runtime_error(GITHUBKEYPATH + " is not valid path");

            std::stringstream githubPrivateKey;
            githubPrivateKey << keyfile.rdbuf();

            auto now = std::chrono::system_clock::now();
            auto iat = now - std::chrono::seconds{60};
            auto exp = now + std::chrono::minutes{9};

            std::string githubToken = jwt::create()
                .set_algorithm("RS256")
                .set_issued_at(iat)
                .set_expires_at(exp)
                .set_issuer(GITHUBCLIENTID)
                .sign(jwt::algorithm::rs256("", githubPrivateKey.str(), "", ""));

            githubLeaveMessage(githubToken, h.pullRequestId(), h.getUserLogin(), h.repository().second, botMsg);
        }
        catch(std::runtime_error &runtimeError)
        {
            std::cerr << runtimeError.what() << std::endl;
        }
        catch(std::exception &otherError)
        {
            std::cerr << otherError.what() << std::endl;
        }
        catch(...)
        {
            std::cerr << "Unable to send message from GitHub Bot" << std::endl;
        }
    }
};
