#include "body_parsers/url_encoded_parser.h"
#include <vector>
#include <string>
#include <sstream>
#include <iostream>

using namespace std;

int URLEncodedParser::Parse(const std::string & body, const HTTPRequestHeader & header, std::map<std::string, Parameter> & body_values, std::map<std::string, ZippyFile> & files){

    if(header.method == "GET"){

        /* https://stackoverflow.com/questions/978061/http-get-with-request-body 
        The message body should be ignored because get shouldn't carry any data*/
        return 200;
    }

    if(header.method == "POST" || header.method == "PUT" || header.method == "PATCH"){
        vector<string> fields_array = {};
        stringstream body_as_ss{body};
        string field;

        /* In URL encoding key-value pairs are delimited by & */
        while(getline(body_as_ss, field, '&')){
            fields_array.push_back(field);
        }

        for(auto it = fields_array.begin(); it != fields_array.end(); ++it){
            auto eq_pos = it->find("=");
            if(eq_pos == std::string::npos){
                /* This means that there is no equal sign, so its not a valid key value pair. It's okay for the value after the equal sign to be empty, but its not ok for there to be no equal sign as it leads to ambiguities in parsing. I considered throwing an error here, but after some research, it seems to be better to just ignore the invalid value*/
            } else{

                auto key = ZippyUtils::URLDecode(it->substr(0, eq_pos));
                auto value = ZippyUtils::URLDecode(it->substr(eq_pos + 1));

                /* We have to handle arrays here. One of the commonest ways of doing this is to repeat the key multiple times with
                different values, so thats what I choose to handle here.*/

                if(body_values.find(key) != body_values.end()){

                    //If the key is already in the map, then we append it
                    body_values[key].Add(value);

                } else{

                    //Otherwise, we create the entry
                    body_values[key] = value;
                }
            }
        }

        return 200;
    }

    return 400;
}