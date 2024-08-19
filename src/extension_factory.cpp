#include "extensions/extension_factory.h"
#include <dlfcn.h>
#include <stdexcept>
#include <exceptions/zippy_exceptions.h>
#include <algorithm>
#include <filesystem>

void ZippyExtensionFactory::LoadExtension(const std::string & path){

    void * handle = dlopen(path.c_str(), RTLD_LAZY);
    if(!handle){
        throw std::runtime_error("Cannot open the extension at " + path);
    }

    ZippyExtensionTypeQueryFunction type_query = (ZippyExtensionTypeQueryFunction) dlsym(handle, "Zippy_QueryExtensionCategory");
    if(!type_query){
        dlclose(handle);
        throw std::runtime_error("Cannot access the extension category query function at " + path);
    }

    std::string category = type_query();
    std::string name = "";
    if(category == "logger"){
        ZippyExtensionNameQueryFunction ext_name_func = (ZippyExtensionNameQueryFunction) dlsym(handle, "Zippy_QueryExtensionName");

        if(!ext_name_func){
            dlclose(handle);
            throw std::runtime_error("Cannot access the extension name query function at " + path);
        }

        name = ext_name_func();

        ZippyLoggerFactoryFunction ff = (ZippyLoggerFactoryFunction) dlsym(handle, "Zippy_CreateExtensionInstance");
        if(!ff){
            dlclose(handle);
            throw std::runtime_error("Cannot access the factory function for the logger at " + path);
        }

        logger_extensions[name] = ff;
        handles.push_back(handle);
    }


}

void ZippyExtensionFactory::LoadExtensionsFromFolder(const std::string & path)
{
    for(const auto & file : std::filesystem::directory_iterator(path)){
        if(file.is_regular_file())
        {
            auto fn = file.path().string();
            auto ext = fn.substr(fn.length() - 3, fn.length());
            if(ext == ".so")
            {
                LoadExtension(fn);
            }
        }
    }

}
    
ILogger* ZippyExtensionFactory::CreateLogger(const std::string & logger_class_name){

    try{
        auto new_logger = logger_extensions.at(logger_class_name)();
        return new_logger;
    } catch(const std::out_of_range & e){
        throw ZippyExtensionNotFoundException{"Could not find logger of type " + logger_class_name};
    }
}

ZippyExtensionFactory::ZippyExtensionFactory(){

}

ZippyExtensionFactory::~ZippyExtensionFactory(){

    std::for_each(handles.begin(), handles.end(), [](void * h) -> void{
        dlclose(h);
    });

}