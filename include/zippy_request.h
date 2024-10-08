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
        std::string boundary;
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

//This is a functor that parses and extracts http headers, so I don't have to hardcode
//it into the ParseHTTPRequest function.
struct HTTPHeaderParser{

        std::string key_name;
        virtual void operator()(HTTPRequestHeader & header, const std::string & header_line) = 0;
        HTTPHeaderParser(const std::string & key);
        virtual ~HTTPHeaderParser();
};

#endif