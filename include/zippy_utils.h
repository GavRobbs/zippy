#ifndef ZIPPY_UTILS_H
#define ZIPPY_UTILS_H

#include <string>
#include <vector>
#include <map>

struct ZippyFile{
        std::string name;
        std::string server_path;
        std::string mime_type;
        std::size_t size;
        std::vector<unsigned char> data;
};

struct Parameter{
        private:

        std::vector<std::string> raw;
        bool isMultiple{false};
        
        public:
        explicit operator int() const;
        explicit operator unsigned int() const;
        explicit operator long() const;
        explicit operator unsigned long() const;
        explicit operator float() const;
        explicit operator double() const;
        explicit operator std::string() const;
        explicit operator std::vector<std::string>() const;
        bool operator==(const Parameter & other) const;
        Parameter();
        Parameter(const std::string &val_in);
        ~Parameter();
        void Add(std::string extra);
};

class ZippyUtils{

    public:
    static std::string BuildHTTPResponse(int status, std::string text_info, std::map<std::string, std::string> headers, std::string body);
    static std::string URLDecode(const std::string & encoded_string);
    static std::string URLEncode(const std::string & decoded_string);
};

#endif