#include "extensions/extension_factory.h"
#include "logging/ilogger.h"
#include <iostream>
#include <memory>

class ConsoleLogger : public ILogger{

    public:
    void Log(const std::string & text){
        std::cout << text << std::endl;
    }

    void Log(const std::string & tag, const std::string & text){
        std::cout << tag << " " << text << std::endl;
    }

    ~ConsoleLogger(){

    }

    ConsoleLogger(){

    }
};

extern "C" std::string Zippy_QueryExtensionCategory(){
    return "logger";
}

extern "C" std::string Zippy_QueryExtensionName(){
    return "ConsoleLogger";
}

extern "C" ILogger * Zippy_CreateExtensionInstance(){
    return new ConsoleLogger{};
}