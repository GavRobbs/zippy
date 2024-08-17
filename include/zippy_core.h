#ifndef ZIPPY_CORE_H
#define ZIPPY_CORE_H

#include <string>
#include <map>
#include <algorithm>
#include <vector>
#include <memory>
#include <ctime>
#include "zippy_utils.h"
#include "zippy_routing.h"
#include "zippy_request.h"

class Connection
{
        public:
        void SendData(std::string data);
        void SendBufferedData();
        std::shared_ptr<HTTPRequest> ReceiveData();
        bool AcceptConnection(const int &serverfd);
        Connection();
        std::string BuildHTTPResponse(int status, std::string text_info, std::map<std::string, std::string> headers, std::string body);
        ~Connection();
        void Close();
        void UpdateLastAliveTime();
        int GetSocketFileDescriptor();
        bool ForClosure();
        
        private:
        int sockfd;
        std::string ip_address;
        HTTPRequest ParseHTTPRequest(const std::string & request_raw);
        void PullData();
        bool HasDataToRead();

        std::time_t last_alive_time;
        HTTPRequestHeader current_headers;

        std::string raw_body;
        std::size_t read_remainder{(std::size_t)-1};
        bool hasToWrite{false};
        bool toClose{false};

        std::vector<std::string> send_buffer;
};

class Application{

        public:
        void Bind(int port);
        void Listen();

        Router & GetRouter();

        Application();
        ~Application();

        private:
        int server_socket_fd;
        Router router;
        std::vector<std::shared_ptr<Connection>> connections;
        void ProcessRequest(std::shared_ptr<HTTPRequest> request, std::shared_ptr<Connection> connection);


};

#endif