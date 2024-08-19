#ifndef ZIPPY_SOCKET_BUFFER_READER_H
#define ZIPPY_SOCKET_BUFFER_READER_H

#include <string>
#include "interfaces/buffer_reader.h"

class SocketBufferReader : public IZippyBufferReader{

    public:
    SocketBufferReader(const int & socket_fd);
    ~SocketBufferReader() noexcept;

    std::optional<std::string> ReadData() override;
    BUFFER_READER_STATUS GetStatus() override;

    private:
    void PullData();
    bool HasDataToRead();
    
    int socket_fd;
    std::string raw_body;
    std::size_t read_remainder{(std::size_t)-1};

    BUFFER_READER_STATUS status;

};

#endif