// Copyright (c) 2026, Alexey Gavrilov


#pragma once

#include <vector>
#include <string>
#include <cstring>
#include <iostream>

#include <nlohmann/json.hpp>

#include "html.h"

class EchoHandler
{
private:
    std::string datastr;
public:
    void acceptRequest(const std::vector<unsigned char> &data)
    {
        std::string str(data.begin(), data.end());

        std::cout << str << std::endl;
    };

    std::vector<unsigned char> getResponse(){return {};};

};

class HttpHandler
{
private:
    constexpr static char PacketDelim[5] = "\r\n\r\n";

    bool isFilled;

    html::Header header;
    nlohmann::json sender;
    size_t prid;
    std::string repositoryUrl;
    int commits;
    std::string headHash, baseHash, repoName;
public:
    bool isBodyValid() {return this->isFilled;}
    void acceptRequest(const std::vector<unsigned char> &data)
    {
        this->isFilled = false;

        std::string packet(data.begin(), data.end());
        packet.resize(std::strlen(packet.c_str()));

        std::string body;

        size_t delimpos = packet.find(PacketDelim);

        if(delimpos != std::string::npos)
        {
            this->header = html::Header(packet.substr(0, delimpos));
            body = packet.substr(sizeof(PacketDelim) / sizeof(char) - 1 + delimpos);
        }

        std::string contentType = header.getValue("content-type");
        if(contentType != "application/json") return;

        nlohmann::json jsonbody = nlohmann::json::parse(body);
        nlohmann::json jsonpayload = nlohmann::json::parse(std::string(jsonbody["payload"]));

        if(header.getValue("x-github-event") == "pull_request"
            and jsonpayload["action"] == "assigned"
        )
        {
            this->sender = jsonpayload["sender"];
            this->prid = jsonpayload["pull_request"]["id"];
            this->repositoryUrl = jsonpayload["html_url"];
            this->commits = jsonpayload["commits"];
            this->baseHash = jsonpayload["base"]["sha"];
            this->headHash = jsonpayload["head"]["sha"];
            this->repoName = jsonpayload["repository"]["name"];
            this->isFilled = true;
        }
    };

    std::vector<unsigned char> getResponse()
    {
        nlohmann::json body;
        body["status"] = "ok";

        std::string response;

        response += header.getValue("version") + " 200 OK\r\n";
        response += "Content-Type: application/json\r\n";
        response += "Content-Length: "
                        + std::to_string(body.dump().size())
                        + "\r\n";
        response += "Connection: close\r\n\r\n";
        response += body.dump(-1, 0);

        return {response.begin(), response.end()};
    };
};
