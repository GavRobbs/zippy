#include "zippy_core.h"
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

std::string Connection::PullData(){

        int numBytes;
        char dataBuffer[4096];

        numBytes = recv(sockfd, dataBuffer, 4095, 0);
        if(numBytes == -1){
                throw std::runtime_error("Error receiving data");
        }

        dataBuffer[numBytes] = '\0';
        return std::string{dataBuffer};
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
        if(HasDataToRead()){
                std::string request_data = PullData();

                if(request_data.empty()){
                        return nullptr;
                }

                HTTPRequest req = ParseHTTPRequest(request_data);
                UpdateLastAliveTime();
                return std::make_shared<HTTPRequest>(req);
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

        std::istringstream stream{request_raw};
        std::vector<std::string> lines{};
        std::string line;
        HTTPRequest request;
        HTTPRequestHeader header;

        while(std::getline(stream, line, '\n')){
                //We removed \n, but we have to remove the \r part of \r\n

                if(line.length() > 1){
                        lines.push_back(line.substr(0, line.length() - 1));
                } else{
                        //This line probably only contains the /r
                        lines.push_back("");
                        break;
                }
        }

        try{
                std::regex re("([A-Z]+)\\s+(\\S+)\\s+HTTP/(\\S+)");
                std::smatch match;
                if(std::regex_match(lines[0], match, re)){
                        header.method = match.str(1);
                        header.path = match.str(2);
                        header.version = match.str(3);
                } else{
                        throw std::runtime_error("Badly formatted first line of HTTP request");
                }
        } catch(std::regex_error & e){
                throw e;
        }

        int q_pindex = header.path.find("?");
        std::string qparams = header.path.substr(q_pindex + 1);
        header.path = header.path.substr(0, q_pindex);

        if(!header.path.length() > 1 && header.path.back() == '/'){
                header.path.erase(header.path.size() - 1);
        }

        stream.clear();
        stream.str(qparams);

        std::regex re2("([^=]+)=(.*)");
        std::smatch m2;

        while(std::getline(stream, line, '&')){

                if(std::regex_match(line, m2, re2)){
                        request.query_params[m2.str(1)] = m2.str(2);
                }
        }

        int first_colon;
        std::string key;
        std::string value;

        for(auto it = lines.begin() + 1; it != lines.end(); it++){

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
                                header.keepAlive = true;
                                keepAlive = true;
                                forceClose = false;
                                //std::cout << "Keep alive is on" << std::endl;
                        } else{
                                header.keepAlive = false;
                                keepAlive = false;
                                forceClose = true;
                        }
                }

        }

        request.header = header;

        int bodyStart = request_raw.find("\r\n\r\n");
        bodyStart += 4;
        request.body = request_raw.substr(bodyStart, request_raw.length());
        current_headers = header;
        return request;
}

void Connection::SendData(std::string data){

        /* If the header is sent with Connection: close, then we close it after a request/response cycle. SendData would
        correspond to the response cycle. */
        if(!current_headers.keepAlive){
                forceClose = true;
        } else{
                UpdateLastAliveTime();
        }

        if(send(sockfd, data.c_str(), data.length(), 0) == -1)
        {
                //std::cout << "Errno is: " << errno << " for " << sockfd << std::endl;
                throw std::runtime_error("Error sending data");
        }

}

void Connection::UpdateLastAliveTime()
{
        if(!current_headers.keepAlive || forceClose){
                return;
        }

        last_alive_time = std::time(0);
}

bool Connection::ForClosure(){

        if(forceClose){
                //std::cout << sockfd << " is force closed." << std::endl;
                return true;
        }

        time_t diff = std::time(0) - last_alive_time;
        return diff > 3;
}

int Connection::GetSocketFileDescriptor(){
        return sockfd;
}


std::string ZippyUtils::BuildHTTPResponse(int status, std::string text_info, std::map<std::string, std::string> headers, std::string body){
        std::stringstream ss;
        ss << "HTTP/1.1 " << status << " " << text_info << "\r\n";
        for(auto it = headers.begin(); it != headers.end(); it++){
                ss << it->first << " : " << it->second << "\r\n";
        }
        ss << "\r\n" << body;
        return ss.str();
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