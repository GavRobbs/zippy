#ifndef ZIPPY_EXCEPTIONS
#define ZIPPY_EXCEPTIONS

#include <stdexcept>

class ZippyKeyNotFoundException : public std::runtime_error{

    public:
    ZippyKeyNotFoundException(std::string error_info);
    ~ZippyKeyNotFoundException();
};

#endif