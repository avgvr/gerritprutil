// Copyright (c) 2026, Alexey Gavrilov


#pragma once

#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <algorithm>
#include <array>
#include <stdexcept>

namespace http
{

class Header
{
private:
    std::map<std::string, std::string> headers;
public:
    Header() = default;

    void operator<<(const std::string &headerContent)
    {
        headers.erase(headers.begin(), headers.end());

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

    std::vector<unsigned char> dump() const
    {
        std::string header = "";
        if(hasHeader("method") and hasHeader("path") and hasHeader("version"))
        {
            header += headers.at("method") + ' ' + headers.at("path") + ' ' + headers.at("version") + "\r\n";
        }
        if(hasHeader("phrase") and hasHeader("code") and hasHeader("version"))
        {
            std::string phraseDelim = headers.at("phrase") == "" ? "" : " ";
            header += headers.at("version") + ' ' + headers.at("code") + phraseDelim + headers.at("phrase") + "\r\n";
        }
        else throw std::runtime_error("HTTP header is invalid");

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

    explicit Http(const std::string &packet)
        : hdr(packet.substr(0, packet.find(PacketDelim))),
        body(
            packet.find(PacketDelim) != std::string::npos ?
                packet.begin() + sizeof(PacketDelim) - 1 +
                    packet.find(PacketDelim):
                packet.end(),
            packet.end()
        ) {};

    const Header& getHttpHeader() const
    {
        return hdr;
    };

    std::pair<std::vector<unsigned char>, std::string> getHttpContent() const
    {
        return {body, hdr.getValue(ContentType)};
    };

    void setHttpContent(std::vector<unsigned char> content, std::string contentType)
    {
        hdr.setHeader(ContentType, contentType);
        body = content;
    };

    void operator<<(const std::string headerContent)
    {
        hdr << headerContent;
        body.clear();
    };

    std::vector<unsigned char> dump() const
    {
        auto packet = hdr.dump();
        packet.insert(packet.end(), body.begin(), body.end());
        return packet;
    };
};

};
