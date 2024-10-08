#ifndef ZIPPY_BODY_PARSER_H
#define ZIPPY_BODY_PARSER_H

#include "zippy_core.h"
#include <map>
#include <string>
class IZippyBodyParser{
    public:
    virtual int Parse(const std::string & body, const HTTPRequestHeader & header, std::map<std::string, Parameter> & body_values, std::map<std::string, ZippyFile> & files) = 0; 
};

#endif
