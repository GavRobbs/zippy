#ifndef ZIPPY_SOCKET_BUFFER_WRITER_H
#define ZIPPY_SOCKET_BUFFER_WRITER_H

#include <vector>
#include <uv.h>
#include "interfaces/buffer_writer.h"

class SocketBufferWriter : public IZippyBufferWriter{

    public:
    SocketBufferWriter(uv_stream_t * cs);
    ~SocketBufferWriter() noexcept;

    void Write(std::vector<unsigned char> data) override;
    BUFFER_WRITER_STATUS GetStatus() override;

    private:
    uv_stream_t * client_socket;

};

#endif