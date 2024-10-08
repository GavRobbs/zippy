#include "body_parsers/multipart_parser.h"
#include "body_parsers/url_encoded_parser.h"
#include <vector>
#include <string>
#include <sstream>
#include <iostream>
#include <regex>

using namespace std;

int MultipartParser::Parse(const std::string & body, const HTTPRequestHeader & header, std::map<std::string, Parameter> & body_values, std::map<std::string, ZippyFile> & files){

    if(header.method == "GET"){
        /* So GET is obviously not a valid method here, but I wasn't sure how to handle the case that it was sent in this manner.
        ChatGPT suggested that I process it as an URL encoded string regardless. */
        return URLEncodedParser().Parse(body, header, body_values, files);
    }

    if(header.method == "POST" || header.method == "PUT" || header.method == "PATCH"){

        vector<string> chunks;
        string chunk;

        size_t start = 0;
        size_t end = 0;
        while((end = body.find("--" + header.boundary, start)) != std::string::npos){
            chunk = body.substr(start, end-start);
            if(chunk != ""){
                chunks.push_back(chunk);
            }
            start = end + header.boundary.length() + 4;
            //We add 4 to account for the -- at the front and the \r\n at the back
        }

        if(ProcessChunks(chunks, body_values, files)){
            return 200;
        }

    }

    return 400;
};

bool MultipartParser::ProcessChunks(std::vector<std::string> & chunks, std::map<std::string, Parameter> & body_values, std::map<std::string, ZippyFile> & files){

    for(auto it = chunks.begin(); it != chunks.end(); ++it){

        if(it->size() == 0){
            continue;
        }

        auto chunk_header_size = it->find("\r\n\r\n");
        string chunk_header = it->substr(0, chunk_header_size + 2);
        string chunk_body = it->substr(chunk_header_size + 4);
        string content_type = "";
        map<string, string> properties;

        //Read the header
        size_t pos = 0;
        size_t old_pos = 0;
        while((pos = chunk_header.find("\r\n", old_pos)) != std::string::npos){

            string chunk_line = chunk_header.substr(old_pos, pos - old_pos);
            old_pos = pos + 2;

            if(chunk_line.find("Content-Disposition:") != std::string::npos){
                std::regex re("(\\w+)\\s*=\\s*\"([^\"]*)\"");
                sregex_iterator next(chunk_line.begin(), chunk_line.end(), re);
                sregex_iterator end;

                while (next != end) {
                    smatch match = *next;
                    properties[match[1]] = match[2];
                    ++next;
                }
            } else if(chunk_line.find("Content-Type:") != std::string::npos){
                regex re("Content-Type:\\s*([a-zA-Z0-9]+/[a-zA-Z0-9\\.\\-\\+]+)");
                smatch match;

                if(regex_search(chunk_line, match, re)){
                    content_type = match[1];
                } else{
                    return false;
                }
                
            }

        }               

        //Make sure this chunk does not contain a file
        if(properties.find("filename") == properties.end()){

            //Trim the whitespace from in front and behind the string
            body_values[properties["name"]] = ZippyUtils::Trim(chunk_body);

        } else{

            ZippyFile zf{"saved_file", properties["filename"], "", content_type, chunk_body.size(), vector<unsigned char>(chunk_body.begin(), chunk_body.end())};
            files[properties["name"]] = zf;
        }
    }

    return true;
}