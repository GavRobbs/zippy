#ifndef ZIPPY_SOCKET_BUFFER_READER_H
#define ZIPPY_SOCKET_BUFFER_READER_H

#include <string>
#include <uv.h>
#include "interfaces/buffer_reader.h"

class SocketBufferReader : public IZippyBufferReader{

    public:
    SocketBufferReader(uv_stream_t * cs);
    ~SocketBufferReader() noexcept;

    void ReadData() override;
    BUFFER_READER_STATUS GetStatus() override;
    void Reset() override;

    private:

    bool detectedContentLength{false};
    bool detectedChunks{false};
    bool finishedHeader{false};
    
    uv_stream_t * client_socket;
    std::string raw_body;
    std::size_t read_remainder{(std::size_t)-1};

    std::size_t body_start_index;
    std::size_t content_length;

    BUFFER_READER_STATUS status;

};

#endif