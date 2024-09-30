#include "extensions/extension_factory.h"
#include "logging/ilogger.h"
#include <iostream>
#include <memory>
#include <chrono>
#include <ctime>
#include <iomanip>

class ConsoleLogger : public ILogger{

    public:
    void Log(const std::string & text){
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        std::tm* local_time = std::localtime(&now_time);

        std::cout << "[" << std::put_time(local_time, "%Y-%m-%d %H:%M:%S") << "] " << text << std::endl;
    }

    void Log(const std::string & tag, const std::string & text){
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        std::tm* local_time = std::localtime(&now_time);

        std::cout << "[" << std::put_time(local_time, "%Y-%m-%d %H:%M:%S") << "] " << tag << ": " << text << std::endl;
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