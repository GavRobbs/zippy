#include <stdexcept>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <iostream>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <regex>
#include <poll.h>
#include "networking/socket_buffer_reader.h"
#include "zippy_core.h"
#include <optional>
#include <string>
#include <sstream>

SocketBufferReader::SocketBufferReader(uv_stream_t * cs):client_socket{cs}, status{IZippyBufferReader::BUFFER_READER_STATUS::IDLE}{

}

SocketBufferReader::~SocketBufferReader(){

}

void SocketBufferReader::ReadData(){

        auto conn =  static_cast<Connection*>(client_socket->data);
        if(conn == NULL || conn->IsForClosure()){
                return;
        }

        //This doesn't do anything if its the first read or it has no keepalive
        //Otherwise it refreshes the timer everytime we try to read new data
        conn->ResetTimer();

        uv_read_start(client_socket, [](uv_handle_t *handle, std::size_t suggested_size, uv_buf_t *buf){

                //This lambda just allocates the data

                buf->base = new char[suggested_size];
                buf->len = suggested_size;
        }, 
        [](uv_stream_t * client, ssize_t nread, const uv_buf_t * buf){

                //Fetch the socket buffer reader object
                auto conn =  static_cast<Connection*>(client->data);
                auto sbr = dynamic_cast<SocketBufferReader*>(conn->GetBufferReader());
                std::size_t body_start = 0;

                if(sbr->status == IZippyBufferReader::BUFFER_READER_STATUS::COMPLETE || sbr->status == IZippyBufferReader::BUFFER_READER_STATUS::INVALID){
                        //If complete, we exit
                        delete [] buf->base;
                        return;
                }

                if(nread > 0){
                        //Read the data
                        conn->ResetTimer();
                        sbr->status = IZippyBufferReader::BUFFER_READER_STATUS::READING;
                        //I was using += here, but apparently it causes errors with
                        //null characters which can happen when you're reading binary
                        sbr->raw_body.append(buf->base, nread);

                        /* If we don't know for sure that we've finished the header, check to see if we have
                        and determine where the body starts.*/
                        if(!sbr->finishedHeader){

                                body_start = sbr->raw_body.find("\r\n\r\n");
                                if(body_start != std::string::npos){
                                        sbr->finishedHeader = true;
                                        sbr->body_start_index = body_start + 4;

                                        //Since we have found the header, we look for hints as to the size of the body content
                                        //Content-Length gives us the size in bytes of the body
                                        //Chunked transfer encoding ends when we reach a chunk with a size of 0
                                        //If there are none, we just read until there's no more data
                                        
                                        if(sbr->raw_body.find("Transfer-Encoding: chunked") != std::string::npos){

                                                sbr->detectedChunks = true;
                                        }
                                        else if(sbr->raw_body.find("Content-Length:") != std::string::npos){

                                                sbr->detectedContentLength = true;

                                                std::regex re("Content-Length:\\s*(\\d+)");
                                                std::smatch match;

                                                std::string header_chunk = sbr->raw_body.substr(0, body_start);

                                                std::regex_search(header_chunk, match, re);
                                                sbr->content_length = std::stol(match[1].str());
                                                conn->GetLogger()->Log("Content length is " + match[1].str());

                                        }else if(sbr->raw_body.find("Connection: close") == std::string::npos){
                                                //You can omit content length in get, head and options requests
                                                //but otherwise, you have to have some indication of length
                                                //for a request with a body to be valid
                                                if(sbr->raw_body.find("GET") != 0 && sbr->raw_body.find("HEAD") != 0 && 
                                                sbr->raw_body.find("OPTIONS") != 0 ){
                                                        sbr->status = IZippyBufferReader::BUFFER_READER_STATUS::INVALID;
                                                }
                                        }
                                }

                        }      
                } else if(nread < 0){
                        if(nread == UV_EOF){
                                sbr->status = IZippyBufferReader::BUFFER_READER_STATUS::COMPLETE;
                                conn->GetLogger()->Log("The request reached EOF!");
                        } else{
                                conn->GetLogger()->Log(std::string{"Read error "} + uv_err_name(nread));
                                sbr->status = IZippyBufferReader::BUFFER_READER_STATUS::INVALID;
                        }
                } else if(nread == 0){
                        conn->ResetTimer();
                        conn->GetLogger()->Log("Not sure if more data is left");
                        //sbr->status = IZippyBufferReader::BUFFER_READER_STATUS::COMPLETE;
                        //TODO: Check the keep-alive here
                }

                if(sbr->finishedHeader){
                        if(sbr->detectedContentLength){
                                //conn->GetLogger()->Log(std::to_string(sbr->raw_body.length() - sbr->body_start_index));
                                if((sbr->raw_body.length() - sbr->body_start_index) >= sbr->content_length){
                                        sbr->status = IZippyBufferReader::BUFFER_READER_STATUS::COMPLETE;
                                } else{
                                        conn->ResetTimer();
                                }
                        } else if(sbr->detectedChunks){
                                if(sbr->raw_body.find("0\r\n\r\n") != std::string::npos){
                                        sbr->status = IZippyBufferReader::BUFFER_READER_STATUS::COMPLETE;
                                }
                        } else{
                                sbr->status = IZippyBufferReader::BUFFER_READER_STATUS::COMPLETE;
                        }

                        if(sbr->status == IZippyBufferReader::BUFFER_READER_STATUS::COMPLETE){
                                //conn->GetLogger()->Log(sbr->raw_body);
                                HTTPRequest request = conn->ParseHTTPRequest(sbr->raw_body);

                                //Here I'm trying to extract the content type from the header
                                std::string head = sbr->raw_body.substr(0, sbr->body_start_index);
                                auto ct_location = head.find("Content-Type:");

                                if(ct_location != std::string::npos){

                                        std::stringstream ct_extractor{head.substr(ct_location + 14)};
                                        std::string ctype = "";
                                        std::getline(ct_extractor, ctype, '\r');

                                        std::size_t mpfd_start = ctype.find("multipart/form-data;");

                                        if(mpfd_start != std::string::npos){
                                                request.header.content_type = "multipart/form-data";
                                                request.header.boundary = ctype.substr(mpfd_start + 30);
                                        } else{
                                                request.header.content_type = ctype;
                                        }

                                }

                                conn->ProcessRequest(request);
                                sbr->Reset();    
                        }
                                            
                }
                

                delete [] buf->base;
        });
        
       
}

IZippyBufferReader::BUFFER_READER_STATUS SocketBufferReader::GetStatus(){
    return status;
}

void SocketBufferReader::Reset(){

        status = IZippyBufferReader::BUFFER_READER_STATUS::IDLE;
        raw_body = "";
        content_length = 0;
        body_start_index = 0;
        detectedContentLength = false;
        detectedChunks = false;
        finishedHeader = false;

}
        