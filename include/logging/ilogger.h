#ifndef ILOGGER_INTERFACE_H
#define ILOGGER_INTERFACE_H

#include <string>

class ILogger{
    public:
    virtual void Log(const std::string & text) = 0;
    virtual void Log(const std::string & tag, const std::string & text) = 0;
    virtual ~ILogger() = default;
};

class NullLogger : public ILogger{

    void Log(const std::string & text) override{};
    void Log(const std::string & tag, const std::string & text) override {};

};

#endif