#ifndef ZIPPY_UTILS_H
#define ZIPPY_UTILS_H

#include <string>
#include <vector>
#include <map>
#include "zippy_exceptions.h"

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

struct NestedKeyValue{

        /* This class is used to store complex attributes in HTTP headers.
        For example, let's say we have Cache-Control: private, max-age=600, must-revalidate.
        The name would be Cache-Control and the nominal value would be private ie. NestedKeyValue{"Cache-Control", Parameter{"private"}}. The subattributes
        would in turn be nested key values NestedKeyValue{"max-age", Parameter{"600"}} and NestedKeyValue{"must-revalidate", Parameter{"true"}}, which
        would then be added to the subattributes of the first NestedKeyValue.       
        */

        Parameter nominal_value;
        std::string name;
        std::vector<NestedKeyValue> subattributes;

        NestedKeyValue operator[](const std::string & parameter_name){
                for(auto it = subattributes.begin(); it != subattributes.end(); ++it){
                        if(it->name == parameter_name){
                                return *it;
                        }
                }

                throw ZippyKeyNotFoundException{parameter_name};
        }

        bool HasAttribute(const std::string & attrib_name){
                for(auto it = subattributes.begin(); it != subattributes.end(); ++it){
                        if(it->name == attrib_name){
                                return true;
                        }
                }

                return false;
        }

        NestedKeyValue(const std::string & key, const Parameter & val):name(key), nominal_value(val){

        }

        ~NestedKeyValue(){

        }
        
        template<class T>
        T Value(){
                return (T)nominal_value;
        }
};

class NestedKeyValueStore{
        /* This acts primarily as a storage class for NestedKeyValue. The idea is that this should go into
        HTTP request headers, and be populated by a chain of header parser middleware, line by line. Going by the example
        above, you would access the max age of Cache-Control header as follows:
                int cache_max_age = headers["Cache-Control"]["max-age"].Value<int>();
        */
        private:
        NestedKeyValue root;

        public:
        NestedKeyValue operator[](const std::string & parameter_name){
                return root[parameter_name];
        }

        void AddNestedKeyValue(const NestedKeyValue & new_pair){
                root.subattributes.push_back(new_pair);
        }
};

class ZippyUtils{

    public:
    static std::string BuildHTTPResponse(int status, std::string text_info, std::map<std::string, std::string> headers, std::string body);
    static std::string URLDecode(const std::string & encoded_string);
    static std::string URLEncode(const std::string & decoded_string);
};

#endif