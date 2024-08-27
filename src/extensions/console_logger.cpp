#include "extensions/extension_factory.h"
#include "logging/ilogger.h"
#include <iostream>
#include <memory>
#include <mutex>

class ConsoleLogger : public ILogger{

    public:
    void Log(const std::string & text){
        std::lock_guard<std::mutex> lock{outputMutex};
        std::cout << text << std::endl;
    }

    void Log(const std::string & tag, const std::string & text){
        std::lock_guard<std::mutex> lock{outputMutex};
        std::cout << tag << " " << text << std::endl;
    }

    ~ConsoleLogger(){

    }

    ConsoleLogger(){

    }

    private:
    std::mutex outputMutex;
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