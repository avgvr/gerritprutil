// Copyright (c) 2026, Alexey Gavrilov


#pragma once

#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <algorithm>
#include <array>
#include <stdexcept>
#include <cstring>

#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>

namespace http
{

class Header
{
private:
    std::map<std::string, std::string> headers;
public:
    Header() = default;

    void erase() {headers.erase(headers.begin(), headers.end());};

    void operator<<(const std::string &headerContent)
    {
        erase();

        constexpr static char HeaderDelim = ':';

        std::stringstream sstream(headerContent);
        std::string entry;

        // Acquire first header line
        constexpr static size_t TKNCOUNT = 3;
        constexpr static const char *reqvhat[TKNCOUNT] = {"version", "path", "method"};
        constexpr static const char *resvhat[TKNCOUNT] = {"phrase", "code", "version"};

        std::getline(sstream, entry, '\r');

        std::stringstream streamhat(entry);
        std::string word = "";

        std::vector<const char*> hatKeys;
        char delim = ' ';
        for(unsigned short it = 0; std::getline(streamhat, word, delim) or it < TKNCOUNT; ++it)
        {
            if(it == 1) delim = '\n';
            if(it == 0 and word.find("HTTP/") != std::string::npos) hatKeys.assign(resvhat, resvhat + TKNCOUNT);
            else if(it == 0 and !word.empty())hatKeys.assign(reqvhat, reqvhat + TKNCOUNT);

            if(!hatKeys.empty())
            {
                headers[hatKeys.back()] = word;
                hatKeys.pop_back();
            }

            word = "";
        }

        std::getline(sstream, word);

        while(std::getline(sstream, entry))
        {
            size_t delim = entry.find(HeaderDelim);
            if(delim != std::string::npos)
            {
                std::string key = entry.substr(0, delim);
                auto crsign = entry.find('\r');
                std::string value = entry.substr(delim + 1, crsign - delim - 1);

                const auto lastcutted = std::remove(value.begin(), value.end(), ' ');
                headers[key] = std::string(value.begin(), lastcutted);
            }
        }
    };

    Header(const std::string &headerUnit)
    {
        this->operator<<(headerUnit);
    };

    std::string getValue(const std::string &header) const
    {
        const auto value = this->headers.find(header);
        if(value == headers.end()) return "";
        else return (*value).second;
    };

    void setHeader(const std::string &header, const std::string &value)
    {
        headers[header] = value;
    };

    bool hasHeader(const std::string &header) const
    {
        return headers.find(header) != headers.end();
    };

    void addBasicAuthentication(const std::string &authstring)
    {
        BIO *bio, *b64;
        BUF_MEM *bufferMem;

        b64 = BIO_new(BIO_f_base64());
        bio = BIO_new(BIO_s_mem());
        bio = BIO_push(b64, bio);

        BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
        BIO_write(bio, authstring.c_str(), authstring.length());
        BIO_flush(bio);
        BIO_get_mem_ptr(bio, &bufferMem);

        std::string auth(bufferMem->data, bufferMem->length);

        headers["Authorization"] = "Basic " + auth;

        BIO_free_all(bio);
    };

    void addBearerAuthentication(const std::string &token)
    {
        headers["Authorization"] = "Bearer " + token;
    };

    std::vector<unsigned char> dump() const
    {
        std::string header = "";
        if(hasHeader("method") and hasHeader("path") and hasHeader("version"))
        {
            header += headers.at("method") + ' ' + headers.at("path") + ' ' + headers.at("version") + "\r\n";
        }
        else if(hasHeader("phrase") and hasHeader("code") and hasHeader("version"))
        {
            std::string phraseDelim = headers.at("phrase") == "" ? "" : " ";
            header += headers.at("version") + ' ' + headers.at("code") + phraseDelim + headers.at("phrase") + "\r\n";
        }
        else throw std::runtime_error("HTTP header is invalid");

        for(const auto& [h, v] : headers)
        {
            if(h == "method" or h == "code" or
                h == "path" or h == "version" or
                h == "phrase")
            {
                continue;
            }
            else
            {
                header += h + ": " + v + "\r\n";
            }
        }

        header += "\r\n";

        std::vector<unsigned char> packet(header.begin(), header.end());
        return packet;
    };

};

class Http
{
private:
    constexpr static char ContentType[] = "Content-Type";

    Header hdr;
    std::vector<unsigned char> body;
public:
    constexpr static char PacketDelim[5] = "\r\n\r\n";

    Http() = default;

    explicit Http(const std::string &packet)
        : hdr(packet.substr(0, packet.find(PacketDelim))),
        body(
            packet.find(PacketDelim) != std::string::npos ?
                packet.begin() + sizeof(PacketDelim) - 1 +
                    packet.find(PacketDelim):
                packet.end(),
            packet.end()
        ) {};

    Header& getHttpHeader()
    {
        return hdr;
    };

    std::pair<const std::vector<unsigned char>&, std::string> getHttpContent() const
    {
        return {body, hdr.getValue(ContentType)};
    };

    void setHttpContent(const std::vector<unsigned char> &content, const std::string &contentType)
    {
        hdr.setHeader(ContentType, contentType);
        body = content;
    };

    void erase() {hdr.erase(); body.clear();};

    void operator<<(const std::vector<unsigned char> &packet)
    {
        std::string packetString(packet.begin(), packet.end());

        size_t packetDelimiter = packetString.find(PacketDelim);
        std::string header =
            std::string(
                packetString.begin(),
                    packetDelimiter != std::string::npos ?
                    packetString.begin() + packetDelimiter :
                    packetString.end());
        std::string content =
            std::string(
                packetDelimiter != std::string::npos ?
                packetString.begin() + packetDelimiter + sizeof(PacketDelim ) - 1:
                packetString.end(),
            packetString.end()
        );

        hdr << header;
        body = std::vector<unsigned char>(content.begin(), content.end());
    };

    std::vector<unsigned char> dump() const
    {
        auto packet = hdr.dump();
        packet.insert(packet.end(), body.begin(), body.end());
        return packet;
    };
};

};
