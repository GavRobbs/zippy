#ifndef ZIPPY_CORE_H
#define ZIPPY_CORE_H

#include <string>
#include <map>
#include <algorithm>
#include <vector>
#include <memory>
#include "zippy_routing.h"

struct HTTPRequestHeader{

        std::string method;
        std::string version;
        std::string path;
        std::string user_agent_info;
        std::string host;
};

struct HTTPRequest{
        HTTPRequestHeader header;
        std::map<std::string, std::string> query_params;
        std::string body;
        std::string ip_address;
};

class ZippyUtils{

    public:
    static std::string BuildHTTPResponse(int status, std::string text_info, std::map<std::string, std::string> headers, std::string body);
};

class Connection
{
        public:
        void SendData(std::string data);
        void ReceiveData();
        void AcceptConnection(const int &serverfd);
        Connection(const int & sock, std::function<void(const HTTPRequest &)> handler);
        std::string BuildHTTPResponse(int status, std::string text_info, std::map<std::string, std::string> headers, std::string body);
        ~Connection();
        void Close();
        bool ForClosure();

        private:
        int sockfd;
        std::string ip_address;
        std::function<void(const HTTPRequest &)> receiver_function;
        HTTPRequest ParseHTTPRequest(const std::string &request_raw);
        std::string PullData();
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

};

#endif