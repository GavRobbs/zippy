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
#include <optional>

SocketBufferReader::SocketBufferReader(const int & socket_fd):socket_fd{socket_fd}, status{IZippyBufferReader::BUFFER_READER_STATUS::IDLE}{

}

SocketBufferReader::~SocketBufferReader(){

}

std::optional<std::string> SocketBufferReader::ReadData(){

    if(read_remainder == -1){

            //The connection is awaiting data
            //if it has data to read pull it
            if(HasDataToRead()){
                PullData();
            }

    } else if(read_remainder == 0){

            //Indicate that we're done reading this current request
            //and we're ready to take another one
            //Return the optional
            read_remainder = -1;
            status = IZippyBufferReader::BUFFER_READER_STATUS::IDLE;

            std::optional<std::string> result;
            result = raw_body;

            return result;
    } else{
            
            //Finish reading the incomplete data
            PullData();

    }

    return std::nullopt;

}


void SocketBufferReader::PullData(){

    if(read_remainder == -1){

            std::size_t numBytes;
            char dataBuffer[8192];

            numBytes = recv(socket_fd, dataBuffer, 8191, 0);
            if(numBytes == -1){
                    if(errno == EAGAIN || errno == EWOULDBLOCK){
                            status = IZippyBufferReader::BUFFER_READER_STATUS::READING;
                            return;
                    } else{
                        status = IZippyBufferReader::BUFFER_READER_STATUS::INVALID;
                        throw std::runtime_error("Error receiving data, errno is " + errno);
                    }
            } else if(numBytes == 0){
                    return;
            }
            dataBuffer[numBytes] = '\0';

            std::string raw_with_header{dataBuffer};

            //Search for content-length headers to get a hint
            int cl_index = raw_with_header.find("Content-Length:");
            
            if(cl_index == std::string::npos){
                    cl_index = raw_with_header.find("content-length:");
            }

            if(cl_index == std::string::npos){
                    //If content length isn't specified, assume it all fits into 8kb
                    //This isn't strictly true, because I need to accomodate chunked data here
                    read_remainder = 0;
                    raw_body = raw_with_header;
                    status = IZippyBufferReader::BUFFER_READER_STATUS::COMPLETE;
                    return;
            }


            //Extract the number for content length
            std::string content_length_number = "";
            std::size_t content_length = 0;

            for(auto it = raw_with_header.begin() + cl_index + 15; it != raw_with_header.end(); ++it){

                    if(*it =='\r' && *(it + 1) == '\n'){
                            //We've hit the end of the line
                            break;
                    }

                    content_length_number += *it;
            }

            content_length = std::stoul(content_length_number);

            //Calculate the header size, the data size and see how much data has been read
            std::size_t header_size = raw_with_header.find("\r\n\r\n") + 4;
            std::size_t data_size = raw_with_header.length() - header_size;
            std::size_t data_read = numBytes - header_size;

            raw_body = raw_with_header;

            if(data_read >= content_length){
                    /* If we've read all the data, we can say there's nothing left */
                    status = IZippyBufferReader::BUFFER_READER_STATUS::COMPLETE;
                    read_remainder = 0;
            } else{
                    /* Otherwise we calculate how much data is left, leaving it for another iteration and return*/
                    read_remainder = content_length - data_read;   
                    status = IZippyBufferReader::BUFFER_READER_STATUS::READING;                     
                    return;
            }

    } else if(read_remainder > 0){

            /* If we didn't get all the data last time, we go here.*/
            
            std::size_t numBytes;
            char dataBuffer[8192];

            numBytes = recv(socket_fd, dataBuffer, 8191, 0);
            if(numBytes == -1){
                    if(errno == EAGAIN || errno == EWOULDBLOCK){
                            status = IZippyBufferReader::BUFFER_READER_STATUS::READING;
                            return;
                    } else{
                            status = IZippyBufferReader::BUFFER_READER_STATUS::INVALID;
                            throw std::runtime_error("Error receiving data, errno is " + errno);
                    }
            }
            dataBuffer[numBytes] = '\0';

            //If there's still more data, we find out how much is left
            //otherwise, we indicate that we've read everything
            //and nothing is left
            if(read_remainder > numBytes){
                    read_remainder -= numBytes;
                    status = IZippyBufferReader::BUFFER_READER_STATUS::READING;
            } else{
                    read_remainder = 0;
                    status = IZippyBufferReader::BUFFER_READER_STATUS::COMPLETE;
            }

            //Append the newly read data to the body
            raw_body += std::string{dataBuffer};
            return;
    } else{
            //If read remainder is 0 then do nothing
    }
}

bool SocketBufferReader::HasDataToRead(){

    pollfd fds[1];
    fds[0].fd = socket_fd;
    fds[0].events = POLLIN;

    int result = poll(fds, 1, 0);
    if(result > 0){
            if(fds[0].revents & POLLIN){
                    return true;
            } else{
                    return false;
            }
    } else{
            return false;
    }

}

IZippyBufferReader::BUFFER_READER_STATUS SocketBufferReader::GetStatus(){
    return status;
}
        