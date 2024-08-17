#ifndef ZIPPY_REQUEST_H
#define ZIPPY_REQUEST_H

#include <string>
#include <map>
#include "zippy_utils.h"

struct HTTPRequestHeader{

        std::string method;
        std::string version;
        std::string path;
        std::string user_agent_info;
        std::string host;
        std::string content_type;
        size_t content_length;
        bool keep_alive;
};

struct HTTPRequest{
        HTTPRequestHeader header;
        std::map<std::string, Parameter> URL_PARAMS;
        std::map<std::string, Parameter> POST;
        std::map<std::string, Parameter> GET;
        std::map<std::string, ZippyFile> FILES;
        std::string body;
        std::string ip_address;
};

#endif