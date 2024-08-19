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
#include <optional>
#include "zippy_core.h"
#include "zippy_utils.h"
#include "networking/socket_buffer_reader.h"

Connection::Connection(const int & sockfd):sockfd(sockfd){
        last_alive_time = std::time(0);
}

Connection::~Connection(){
}

void Connection::Close(){
        //std::cout << "Closing connection identified by " << sockfd << std::endl;
        close(sockfd);
}

std::shared_ptr<HTTPRequest> Connection::ReceiveData(const Application & app){

        if(reader == nullptr){
                return nullptr;
        }

        if((!reader->GetStatus()) == IZippyBufferReader::BUFFER_READER_STATUS::INVALID){
                UpdateLastAliveTime();
        }
        
        std::optional<std::string> data = reader->ReadData();
        if(!data.has_value()){
                return nullptr;
        }

        return std::make_shared<HTTPRequest>(ParseHTTPRequest(data.value(), app));        

}

HTTPRequest Connection::ParseHTTPRequest(const std::string &request_raw, const Application & app){

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

void Connection::SetBufferReader(std::shared_ptr<IZippyBufferReader> reader){
        this->reader = reader;
}


void Application::Bind(int port){

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
                logger->Log("Zippy application bound to port " + port);
        }

}
        
void Application::Listen(){


        pollfd fds[1];
        fds[0].fd = server_socket_fd;
        fds[0].events = POLLIN;

        int result = poll(fds, 1, 0);
        if(result > 0){
                if(fds[0].revents & POLLIN){

                        while(true){

                                auto new_conn = AcceptConnection();

                                if(new_conn.has_value()){

                                        std::shared_ptr<Connection> connection = std::make_shared<Connection>(new_conn.value());
                                        connection->SetBufferReader(std::make_shared<SocketBufferReader>(new_conn.value()));
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
                auto r = (*it)->ReceiveData(*this);

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

        SetLogger(std::make_unique<NullLogger>());

}

Application::~Application(){
        close(server_socket_fd);
}

void Application::ProcessRequest(std::shared_ptr<HTTPRequest> request, std::shared_ptr<Connection> connection){

        RouteFunction func;

        if(request == nullptr){
                return;
        }

        logger->Log("Request path is: " + request->header.path);

        if(router.FindRoute(request->header.path, func, request->URL_PARAMS)){
                connection->SendData(func(*request));
        } else{
                std::string data = ZippyUtils::BuildHTTPResponse(404, "Not found.", {}, "");
                connection->SendData(data);
        }
}

void Application::AddHeaderParser(std::shared_ptr<HTTPHeaderParser> parser){

        header_parsers[parser->key_name] = parser;

}

std::weak_ptr<HTTPHeaderParser> Application::GetHeaderParsers(const std::string & header_name){

        std::weak_ptr<HTTPHeaderParser> weakPtr = header_parsers[header_name];
        return weakPtr;

}

std::optional<int> Application::AcceptConnection(){

        socklen_t sin_size;
        sockaddr_storage their_address;
        int sockfd = accept(server_socket_fd, (sockaddr*)&their_address, &sin_size);
        std::string ip_address;

        int flags = fcntl(sockfd, F_GETFL, 0);
        fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

        if(sockfd < 0){
                return std::nullopt;
        }

        if(their_address.ss_family == AF_INET){

                char ipv4_address[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &((sockaddr_in*)&their_address)->sin_addr, ipv4_address, INET_ADDRSTRLEN);
                ip_address = ipv4_address;

        } else{

                char ipv6_address[INET6_ADDRSTRLEN];
                inet_ntop(AF_INET, &((sockaddr_in6*)&their_address)->sin6_addr, ipv6_address, INET6_ADDRSTRLEN);
                ip_address = ipv6_address;
        }

        logger->Log("Connection accepted from " + ip_address);
        return sockfd;

}

void Application::SetLogger(std::unique_ptr<ILogger> logger){
        this->logger = std::move(logger);
}
