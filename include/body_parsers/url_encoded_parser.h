#ifndef ZIPPY_URL_ENCODED_PARSER_H
#define ZIPPY_URL_ENCODED_PARSER_H

#include "interfaces/body_parser.h"

class URLEncodedParser : public IZippyBodyParser{

    public:
    virtual int Parse(const std::string & body, const HTTPRequestHeader & header, std::map<std::string, Parameter> & body_values, std::map<std::string, ZippyFile> & files) override; 
};

#endif