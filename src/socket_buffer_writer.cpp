#include "networking/socket_buffer_writer.h"
#include "zippy_core.h"

SocketBufferWriter::SocketBufferWriter(uv_stream_t * cs):client_socket{cs}{

}
    
SocketBufferWriter::~SocketBufferWriter(){

}

void SocketBufferWriter::Write(std::vector<unsigned char> data){

    uv_buf_t buf = uv_buf_init(reinterpret_cast<char*>(data.data()), data.size());
    uv_write_t* write_request = new uv_write_t;
    write_request->data = client_socket;
    uv_write(write_request, client_socket, &buf, 1, [](uv_write_t* wr, int status){

        auto cs = static_cast<uv_stream_t*>(wr->data);
        auto conn = static_cast<Connection*>(cs->data);

        if(status < 0){
            conn->GetLogger()->Log(std::string{"Write error: "} + uv_strerror(status));
        } else{
            conn->GetLogger()->Log("Wrote data!");
        }

        delete wr;

    });

}
    
IZippyBufferWriter::BUFFER_WRITER_STATUS SocketBufferWriter::GetStatus(){

    return BUFFER_WRITER_STATUS::COMPLETE;

}