#ifndef ZIPPY_CORE_H
#define ZIPPY_CORE_H

#include <string>
#include <map>
#include <algorithm>
#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <ctime>
#include <mutex>
#include <atomic>
#include <optional>
#include <condition_variable>
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
        std::shared_ptr<HTTPRequest> ReceiveData();
        Connection(const int &sockfd, Router & r, std::shared_ptr<ILogger> logger);
        Connection(const Connection &) = delete;
        Connection & operator=(const Connection &) = delete;
        Connection(Connection && other) noexcept;
        Connection & operator=(Connection && other) noexcept;
        ~Connection();
        void Close();
        void UpdateLastAliveTime();
        int GetSocketFileDescriptor();
        bool ForClosure();
        void operator()();

        void SetBufferReader(std::unique_ptr<IZippyBufferReader> reader);
        std::string BuildHTTPResponse(int status, std::string text_info, std::map<std::string, std::string> headers, std::string body);

        private:
        int sockfd;
        std::string ip_address;

        std::time_t last_alive_time;
        HTTPRequestHeader current_headers;

        bool hasToWrite{false};
        bool toClose{false};

        std::vector<std::string> send_buffer;
        std::shared_ptr<ILogger> logger;
        std::unique_ptr<IZippyBufferReader> reader;
        Router & router;

        HTTPRequest ParseHTTPRequest(const std::string & request_raw);
        void ProcessRequest(std::shared_ptr<HTTPRequest> request);
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
        
        std::vector<std::thread> connection_thread_pool;
        std::queue<std::unique_ptr<Connection>> connections;
        std::map<std::string, std::shared_ptr<HTTPHeaderParser>> header_parsers;
        std::optional<int> AcceptConnection();
        std::shared_ptr<ILogger> logger;
        std::atomic<bool> stopApplicationFlag{false};
        std::mutex connection_queue_mutex;
        std::condition_variable condition;


};

#endif