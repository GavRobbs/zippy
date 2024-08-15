#include "zippy_core.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <iostream>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <cstring>
#include <unistd.h>
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

Connection::Connection(const int & sock, std::function<void(const HTTPRequest &)> handler):sockfd(sock), receiver_function(handler){

}

Connection::~Connection(){
        if(sockfd != -1){
                close(sockfd);
        }
}

void Connection::Close(){
        if(sockfd != -1){
                close(sockfd);
        }
        sockfd = -1;
}

void Connection::AcceptConnection(const int &serverfd){

        socklen_t sin_size;
        sockaddr_storage their_address;
        sockfd = accept(serverfd, (sockaddr*)&their_address, &sin_size);

        if(sockfd < 0){
                throw std::runtime_error("Connection not accepted properly");
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

}

void Connection::ReceiveData(){
        std::string request_data = PullData();
        HTTPRequest req = ParseHTTPRequest(request_data);
        receiver_function(req);
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

                value = it->substr(first_colon + 1, it->length());

                if(key == "User-Agent"){
                        header.user_agent_info = value;
                } else if(key == "Host"){
                        header.host = value;
                }

        }

        request.header = header;

        int bodyStart = request_raw.find("\r\n\r\n");
        bodyStart += 4;
        request.body = request_raw.substr(bodyStart, request_raw.length());
        return request;
}

void Connection::SendData(std::string data){

        if(send(sockfd, data.c_str(), data.length(), 0) == -1)
        {
                throw std::runtime_error("Error sending data");
        }

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


        int result = poll(fds, 1, timeout);
        if(result > 0){
                if(fds[0].revents & POLLIN){

                        while(true){

                                std::shared_ptr<Connection> connection = std::make_shared(server_socket_fd, [this, &connection](const HTTPRequest & request){

                                        RouteFunction func;

                                        if(this->router.FindRoute(request.header.path, func)){
                                                connection.SendData(func(request));
                                        } else{
                                                std::string data = ZippyUtils::BuildHTTPResponse(404, "Not found.", {}, "");
                                                connection.SendData(data);
                                        }
                                });

                                try{
                                        connection->AcceptConnection(server_socket_fd);
                                        connections->push_back(connection);
                                } catch(const std::runtime_error & e){
                                        if(errno == EWOULDBLOCK || errno == EAGAIN){
                                                break;
                                        } else{
                                                throw std::runtime_error("There was an unspecified error with a connection");
                                        }
                                }

                        }                        
                }
        } else if(result == 0){

                //There are no pending connections

        } else{
                throw std::runtime_error("Polling error when checking connections");
        }

        //We iterate through all the connections, close those that need to be closed
        //and remove them from the array
        //If they are not to be closed, then we receive data from them
        for(auto it = connections.begin(); it != connections.end(); ){
                if((*it)->ForClosure()){
                        (*it)->Close();
                        it = vec.erase(it);
                } else{
                        (*it)->ReceiveData();
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