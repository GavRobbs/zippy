#ifndef ZIPPY_MULTIPART_PARSER_H
#define ZIPPY_MULTIPART_PARSER_H

#include "interfaces/body_parser.h"

class MultipartParser : public IZippyBodyParser{

    public:
    virtual int Parse(const std::string & body, const HTTPRequestHeader & header, std::map<std::string, Parameter> & body_values, std::map<std::string, ZippyFile> & files) override;

    private:
    bool ProcessChunks(std::vector<std::string> & chunks, std::map<std::string, Parameter> & body_values, std::map<std::string, ZippyFile> & files);
};

#endif