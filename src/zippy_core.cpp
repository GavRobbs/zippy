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
#include "zippy_core.h"
#include "zippy_utils.h"

void Connection::PullData(){

        if(read_remainder == -1){

                std::size_t numBytes;
                char dataBuffer[8192];

                numBytes = recv(sockfd, dataBuffer, 8191, 0);
                if(numBytes == -1){
                        if(errno == EAGAIN || errno == EWOULDBLOCK){
                                return;
                        } else{
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
                        read_remainder = 0;
                        raw_body = raw_with_header;
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
                        read_remainder = 0;
                } else{
                        /* Otherwise we calculate how much data is left, leaving it for another iteration and return*/
                        read_remainder = content_length - data_read;                        
                        return;
                }

        } else if(read_remainder > 0){

                /* If we didn't get all the data last time, we go here.*/
                
                std::size_t numBytes;
                char dataBuffer[8192];

                numBytes = recv(sockfd, dataBuffer, 8191, 0);
                if(numBytes == -1){
                        if(errno == EAGAIN || errno == EWOULDBLOCK){
                                return;
                        } else{
                                throw std::runtime_error("Error receiving data, errno is " + errno);
                        }
                }
                dataBuffer[numBytes] = '\0';

                //If there's still more data, we find out how much is left
                //otherwise, we indicate that we've read everything
                //and nothing is left
                if(read_remainder > numBytes){
                        read_remainder -= numBytes;
                } else{
                        read_remainder = 0;
                }

                //Append the newly read data to the body
                raw_body += std::string{dataBuffer};
                return;
        } else{
                //If read remainder is 0 then do nothing
        }

        

}

Connection::Connection(){

        last_alive_time = std::time(0);

}

Connection::~Connection(){
}

void Connection::Close(){
        //std::cout << "Closing connection identified by " << sockfd << std::endl;
        close(sockfd);
}

bool Connection::AcceptConnection(const int &serverfd){

        socklen_t sin_size;
        sockaddr_storage their_address;
        sockfd = accept(serverfd, (sockaddr*)&their_address, &sin_size);

        int flags = fcntl(sockfd, F_GETFL, 0);
        fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

        if(sockfd < 0){
                return false;
        } else{

                if(their_address.ss_family == AF_INET){

                char ipv4_address[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &((sockaddr_in*)&their_address)->sin_addr, ipv4_address, INET_ADDRSTRLEN);
                ip_address = ipv4_address;

                } else{

                        char ipv6_address[INET6_ADDRSTRLEN];
                        inet_ntop(AF_INET, &((sockaddr_in6*)&their_address)->sin6_addr, ipv6_address, INET6_ADDRSTRLEN);
                        ip_address = ipv6_address;
                }
        }

        std::cout << "Connection accepted from " << ip_address << std::endl;
        return true;

}

std::shared_ptr<HTTPRequest> Connection::ReceiveData(){

        if(read_remainder == -1){

                //The connection is awaiting data
                //if it has data to read pull it
                if(HasDataToRead()){
                        PullData();
                }

        } else if(read_remainder == 0){

                UpdateLastAliveTime();

                //Indicate that we're done reading this current request
                //and we're ready to take another one
                read_remainder = -1;

                //Get the http request data
                try{
                        HTTPRequest req = ParseHTTPRequest(raw_body);
                        return std::make_shared<HTTPRequest>(req);
                } catch(const std::runtime_error & e){
                        std::cout << "Empty request avoided" << std::endl;
                        return nullptr;
                }
        } else{
                
                UpdateLastAliveTime();
                //Finish reading the incomplete data
                PullData();

        }

        return nullptr;
}

bool Connection::HasDataToRead()
{
        pollfd fds[1];
        fds[0].fd = sockfd;
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

HTTPRequest Connection::ParseHTTPRequest(const std::string &request_raw){

        if(request_raw == ""){
                throw std::runtime_error("Empty request passed for parsing");
        }

        std::vector<std::string> header_lines{};
        std::string header_line;
        HTTPRequest request;
        HTTPRequestHeader header;

        std::string header_raw, body_raw;

        std::size_t separating_line_index = request_raw.find("\r\n\r\n");
        header_raw = request_raw.substr(0, separating_line_index + 2); //We want to get the last \r\n\r\n in the header
        body_raw = request_raw.substr(separating_line_index + 4, request_raw.length());

        //We read the header as a bunch of lines for later processing
        std::istringstream stream{header_raw};

        while(std::getline(stream, header_line, '\n')){
                //We removed \n, but we have to remove the \r part of \r\n

                if(header_line.length() > 1){
                        header_lines.push_back(header_line.substr(0, header_line.length() - 1));
                } else{
                        //This line probably only contains the /r
                        header_lines.push_back("");
                        break;
                }
        }

        //Get the URL from the request
        try{
                std::regex re("([A-Z]+)\\s+(\\S+)\\s+HTTP/(\\S+)");
                std::smatch match;
                if(std::regex_match(header_lines[0], match, re)){
                        header.method = match.str(1);
                        header.path = match.str(2);
                        header.version = match.str(3);
                } else{
                        throw std::runtime_error("Badly formatted first line of HTTP request");
                }
        } catch(std::regex_error & e){
                throw e;
        }

        //Get the query parameters from the URL
        int q_pindex = header.path.find("?");
        std::string qparams = header.path.substr(q_pindex + 1);
        header.path = header.path.substr(0, q_pindex);

        if(!header.path.length() > 1 && header.path.back() == '/'){
                header.path.erase(header.path.size() - 1);
        }

        stream.clear();
        stream.str(qparams);

        //Support encoding of query parameters as either & or ;
        char delimiter = ' ';
        if(qparams.find('&') != std::string::npos){
                delimiter = '&';
        } else{
                delimiter = ';';
        }

        std::regex re2("([^=]+)=(.*)");
        std::smatch m2;

        //TODO: Work on URL encoding

        while(std::getline(stream, header_line, delimiter)){

                if(std::regex_match(header_line, m2, re2)){

                        if(request.GET.find(m2.str(1)) == request.GET.end()){
                                //If the entry doesn't already exist, then add it
                                request.GET[m2.str(1)] = Parameter{ZippyUtils::URLDecode(m2.str(2))};
                        } else{
                                //If it exists, it means we passed the same query parameter multiple times
                                //so we're looking at an array
                                //Its up to users to handle this and make sure its formatted properly
                                request.GET[m2.str(1)].Add(ZippyUtils::URLDecode(m2.str(2)));
                        }
                }
        }

        //Parse the lines from the header for each option

        int first_colon;
        std::string key;
        std::string value;

        for(auto it = header_lines.begin() + 1; it != header_lines.end(); it++){

                //We would have already passed the first line with the verb and path

                if(it->length() == 0){
                        //This is the delimiter before the body, not processing that yet so break
                        break;
                }

                first_colon = it->find(":");

                if(first_colon == std::string::npos){
                        throw std::runtime_error("Invalid header formatting");
                }
                
                key = it->substr(0, first_colon);
                std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c){
                        return std::tolower(c);
                });

                //+2 to trim the first space in front of the character
                value = it->substr(first_colon + 2, it->length());
                std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c){
                        return std::tolower(c);
                });

                //std::cout << key << ": " << value << std::endl;

                if(key == "user-agent"){
                        header.user_agent_info = value;
                } else if(key == "host"){
                        header.host = value;
                } else if(key == "connection"){

                        if(value == "keep-alive"){
                                header.keep_alive = true;
                                //std::cout << "Keep alive is on" << std::endl;
                        } else{
                                header.keep_alive = false;
                        }
                } else if(key == "content-length"){
                        header.content_length = std::stoul(value);
                }

        }

        request.header = header;
        request.body = body_raw;
        current_headers = header;
        return request;
}

void Connection::SendBufferedData(){

        //If the buffers are empty, return

        UpdateLastAliveTime();

        if(send_buffer.empty()){

                hasToWrite = false;
                return;
        }

        size_t bytes_sent{0};

        auto it = send_buffer.begin();

        //Send bytes via the socket
        bytes_sent = send(sockfd, it->c_str(), it->length(), 0);

        //Check if there was an error sending the data
        if(bytes_sent == -1){
                throw std::runtime_error("Error sending data");
        }

        std::cout << "Sent " << bytes_sent << " bytes " << std::endl;

        //if the number of bytes sent was less than the length of the entry
        //chop up the string so we have the remainder to send next time
        //Otherwise, erase it
        if(bytes_sent < it->length()){
                *it = it->substr(bytes_sent, it->length());
        } else{
                send_buffer.erase(it);
        }

        if(send_buffer.empty()){
                hasToWrite = false;
                toClose = true;
        } else{
                hasToWrite = true;
        }
}

void Connection::SendData(std::string data){

        UpdateLastAliveTime();
        send_buffer.push_back(data);

}

void Connection::UpdateLastAliveTime()
{
        if(!current_headers.keep_alive){
                return;
        }

        last_alive_time = std::time(0);
}

bool Connection::ForClosure(){

        if(!current_headers.keep_alive){
                return toClose;
        }        

        time_t diff = std::time(0) - last_alive_time;
        return diff > 5;
}

int Connection::GetSocketFileDescriptor(){
        return sockfd;
}

void Application::Bind(int port){

        char ipstr[INET6_ADDRSTRLEN];
        int yes = 1;

        sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

        if((server_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) <= 0){
                throw std::runtime_error("Could not get socket!");
        }

        if (setsockopt(server_socket_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &yes, sizeof(yes))) {
                close(server_socket_fd);
                throw std::runtime_error("Setting socket options failed");
        }

        //This makes our server non-blocking
        int flags = fcntl(server_socket_fd, F_GETFL, 0);
        fcntl(server_socket_fd, F_SETFL, flags | O_NONBLOCK);

        if(bind(server_socket_fd, (sockaddr*)&address, sizeof(address)) < 0){
                close(server_socket_fd);
                throw std::runtime_error("Failed to bind socket to port " + port);
        }


        if(listen(server_socket_fd, 50) == -1){
                close(server_socket_fd);
                throw std::runtime_error("Failed to start listening on port " + port);
        } else{ 
                std::cout << "Zippy application bound to port " << port << std::endl;
        }

}
        
void Application::Listen(){


        pollfd fds[1];
        fds[0].fd = server_socket_fd;
        fds[0].events = POLLIN;

        int timeout = 2500;

        int result = poll(fds, 1, 0);
        if(result > 0){
                if(fds[0].revents & POLLIN){

                        while(true){

                                std::shared_ptr<Connection> connection = std::make_shared<Connection>();

                                if(connection->AcceptConnection(server_socket_fd)){
                                        connections.push_back(connection);
                                } else{
                                        break;
                                }                                

                        }                        
                }
        } else if(result == 0){

        } else{
                throw std::runtime_error("Polling error when checking connections");
        }

        //We iterate through all the connections, close those that need to be closed
        //and remove them from the array
        //If they are not to be closed, then we receive data from them
        for(auto it = connections.begin(); it != connections.end(); ){
                auto r = (*it)->ReceiveData();

                if(r != nullptr){
                        ProcessRequest(r, *it);
                }

                (*it)->SendBufferedData();

                if((*it)->ForClosure()){
                        (*it)->Close();
                        it = connections.erase(it);
                } else{
                        ++it;
                }
        }
}

Router & Application::GetRouter()
{
        return router;
}

Application::Application():router(Router{}){

}

Application::~Application(){
        close(server_socket_fd);
}

void Application::ProcessRequest(std::shared_ptr<HTTPRequest> request, std::shared_ptr<Connection> connection){

        RouteFunction func;

        if(request == nullptr){
                return;
        }

        if(router.FindRoute(request->header.path, func)){
                connection->SendData(func(*request));
        } else{
                std::string data = ZippyUtils::BuildHTTPResponse(404, "Not found.", {}, "");
                connection->SendData(data);
        }
}