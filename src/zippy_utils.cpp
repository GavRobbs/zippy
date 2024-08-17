#include "zippy_utils.h"
#include <sstream>
#include <iomanip>

void Parameter::Add(std::string extra){

    if(!isMultiple){
        isMultiple = true;
    }

    raw.push_back(extra);
}

Parameter::operator int() const{

    return std::stoi(raw[0]);

}

Parameter::operator unsigned int() const{
    return std::stoul(raw[0]);    
}

Parameter::operator long() const{
    return std::stol(raw[0]);    
}

Parameter::operator unsigned long() const{
    return std::stoul(raw[0]);    
}

Parameter::operator float() const{
    return std::stof(raw[0]);
}

Parameter::operator double() const{
    return std::stod(raw[0]);
}

Parameter::operator std::string() const{
    return raw[0];
}

Parameter::operator std::vector<std::string>() const{
    return raw;
}

Parameter::Parameter():raw{}{

}

Parameter::Parameter(const std::string &val_in):raw{{val_in}}{

}

Parameter::~Parameter(){

}

bool Parameter::operator==(const Parameter & other) const{

    return other.raw == this->raw && other.isMultiple == this->isMultiple;
}


std::string ZippyUtils::BuildHTTPResponse(int status, std::string text_info, std::map<std::string, std::string> headers, std::string body){
        std::stringstream ss;
        ss << "HTTP/1.1 " << status << " " << text_info << "\r\n";
        ss << "Content-Length: " << body.length() << "\r\n";
        for(auto it = headers.begin(); it != headers.end(); it++){
                ss << it->first << " : " << it->second << "\r\n";
        }
        ss << "\r\n" << body;
        return ss.str();
}

std::string ZippyUtils::URLDecode(const std::string & encoded_string){
    std::istringstream ss{encoded_string};
    std::string decoded;
    char c;

    std::string hex = "";
    char c1, c2;

    while(ss.get(c)){

        if(c == '+'){
            decoded += ' ';
        } else if(c == '%'){
            ss.get(c1);
            ss.get(c2);
            hex += c1;
            hex += c2;
            decoded += static_cast<char>(std::stoi(hex, nullptr, 16));
        } else{
            decoded += c;
        } 
    }

    return decoded;
}

std::string ZippyUtils::URLEncode(const std::string & decoded_string){
    
    std::istringstream ss{decoded_string};
    std::stringstream output;
    char next;

    output << std::hex << std::setw(2) << std::setfill('0');

    while(ss.get(next)){

        if(next == ' '){
            output << "+";
            continue;
        }

        int val = static_cast<int>(next);
        if((val >= 0x30 && val <= 0x39) || (val >= 0x41 && val <= 0x5A) || (val >= 0x61 && val <= 0x7A)){
            output << next;
        } else{
            output << "%" << val;
        }
    }

    return output.str();
}