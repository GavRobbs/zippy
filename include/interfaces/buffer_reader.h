#ifndef ZIPPY_BUFFER_READER
#define ZIPPY_BUFFER_READER

#include <string>
#include <optional>

/* This class defines the interface for the buffer reader used in a connection. I made it an interface primarily for testing purposes,
but it would also be good for reading other types of data such as websockets, if I ever implement that. */

class IZippyBufferReader{

    public:

    enum BUFFER_READER_STATUS : int { IDLE, READING, COMPLETE, INVALID };

    virtual std::optional<std::string> ReadData() = 0;
    virtual BUFFER_READER_STATUS GetStatus() = 0;
    virtual ~IZippyBufferReader() noexcept = default;
};

#endif