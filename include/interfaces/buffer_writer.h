#ifndef ZIPPY_BUFFER_WRITER_H
#define ZIPPY_BUFFER_WRITER_H

#include <vector>
/* This class defines the buffer writer. */

class IZippyBufferWriter{

    public:

    enum BUFFER_WRITER_STATUS : int { IDLE, WRITING, COMPLETE, INVALID };

    virtual void Write(std::vector<unsigned char> data) = 0;
    virtual BUFFER_WRITER_STATUS GetStatus() = 0;
    virtual ~IZippyBufferWriter() noexcept = default;
};

#endif