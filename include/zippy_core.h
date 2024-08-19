#ifndef ZIPPY_CORE_H
#define ZIPPY_CORE_H

#include <string>
#include <map>
#include <algorithm>
#include <vector>
#include <memory>
#include <ctime>
#include <optional>
#include "zippy_utils.h"
#include "zippy_routing.h"
#include "zippy_request.h"
#include "interfaces/buffer_reader.h"
#include "logging/ilogger.h"

class Application;

class Connection
{
        public:
        void SendData(std::string data);
        void SendBufferedData();
        std::shared_ptr<HTTPRequest> ReceiveData(const Application & app);
        Connection(const int &sockfd);
        std::string BuildHTTPResponse(int status, std::string text_info, std::map<std::string, std::string> headers, std::string body);
        ~Connection();
        void Close();
        void UpdateLastAliveTime();
        int GetSocketFileDescriptor();
        bool ForClosure();

        void SetBufferReader(std::shared_ptr<IZippyBufferReader> reader);
        
        private:
        int sockfd;
        std::string ip_address;
        HTTPRequest ParseHTTPRequest(const std::string & request_raw, const Application & app);

        std::time_t last_alive_time;
        HTTPRequestHeader current_headers;

        bool hasToWrite{false};
        bool toClose{false};

        std::vector<std::string> send_buffer;
        std::shared_ptr<IZippyBufferReader> reader;
};

class Application{

        public:
        void Bind(int port);
        void Listen();

        Router & GetRouter();

        Application();
        ~Application();

        void AddHeaderParser(std::shared_ptr<HTTPHeaderParser> parser);
        std::weak_ptr<HTTPHeaderParser> GetHeaderParsers(const std::string & header_name);
        void SetLogger(std::unique_ptr<ILogger> logger);

        private:
        int server_socket_fd;
        Router router;
        std::vector<std::shared_ptr<Connection>> connections;
        std::map<std::string, std::shared_ptr<HTTPHeaderParser>> header_parsers;
        void ProcessRequest(std::shared_ptr<HTTPRequest> request, std::shared_ptr<Connection> connection);
        std::optional<int> AcceptConnection();
        std::unique_ptr<ILogger> logger;


};

#endif