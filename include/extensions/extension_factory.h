#ifndef ZIPPY_EXTENSION_FACTORY_H
#define ZIPPY_EXTENSION_FACTORY_H

#include "logging/ilogger.h"
#include <memory>
#include <vector>
#include <map>

typedef ILogger* (*ZippyLoggerFactoryFunction)();
typedef std::string (*ZippyExtensionTypeQueryFunction)();
typedef ZippyExtensionTypeQueryFunction ZippyExtensionNameQueryFunction;

class ZippyExtensionFactory{

    public:

    void LoadExtension(const std::string & path);
    void LoadExtensionsFromFolder(const std::string & path);
    ILogger * CreateLogger(const std::string & logger_class_name);

    ZippyExtensionFactory();
    ~ZippyExtensionFactory();

    private:

    std::map<std::string, ZippyLoggerFactoryFunction> logger_extensions{};
    std::vector<void *> handles{};

};


#endif