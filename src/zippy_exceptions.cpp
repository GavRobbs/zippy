#include "exceptions/zippy_exceptions.h"

ZippyKeyNotFoundException::ZippyKeyNotFoundException(std::string error_info):std::runtime_error{error_info}{
}

ZippyKeyNotFoundException::~ZippyKeyNotFoundException(){

}