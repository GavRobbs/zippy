#ifndef ZIPPY_BUFFER_READER_H
#define ZIPPY_BUFFER_READER_H

#include <string>
#include <optional>

/* This class defines the interface for the buffer reader used in a connection. I made it an interface primarily for testing purposes,
but it would also be good for reading other types of data such as websockets, if I ever implement that. */

class IZippyBufferReader{

    public:

    enum BUFFER_READER_STATUS : int { IDLE, READING, COMPLETE, INVALID };

    virtual void ReadData() = 0;
    virtual BUFFER_READER_STATUS GetStatus() = 0;
    virtual void Reset() = 0;
    virtual ~IZippyBufferReader() noexcept = default;
};

#endif