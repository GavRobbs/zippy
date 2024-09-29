#include "extensions/extension_factory.h"
#include "logging/ilogger.h"
#include <iostream>
#include <memory>
#include <fstream>
#include <mutex>

class FileLogger : public ILogger{

    public:
    void Log(const std::string & text){
        file << text << std::endl;
    }

    void Log(const std::string & tag, const std::string & text){
        file << tag << " " << text << std::endl;
    }

    ~FileLogger(){

        file.close();

    }

    FileLogger(){

        file.open("log.txt", std::ios::app);

    }

    private:
    std::ofstream file;
};

extern "C" std::string Zippy_QueryExtensionCategory(){
    return "logger";
}

extern "C" std::string Zippy_QueryExtensionName(){
    return "FileLogger";
}

extern "C" ILogger * Zippy_CreateExtensionInstance(){
    return new FileLogger{};
}