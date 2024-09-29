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
#include <uv.h>
#include <condition_variable>
#include "zippy_utils.h"
#include "zippy_routing.h"
#include "zippy_request.h"
#include "interfaces/buffer_reader.h"
#include "interfaces/buffer_writer.h"
#include "logging/ilogger.h"

class Application;

class Connection
{
        public:
        void SendData(std::string data);
        void ReadData();
        Connection(uv_tcp_t * conn, Router & r, std::shared_ptr<ILogger> logger);
        Connection(const Connection &) = delete;
        Connection & operator=(const Connection &) = delete;
        Connection(Connection && other) noexcept;
        Connection & operator=(Connection && other) noexcept;
        ~Connection();
        void Close();
        void UpdateLastAliveTime();
        bool ForClosure();

        void SetBufferReader(std::unique_ptr<IZippyBufferReader> reader);
        void SetBufferWriter(std::unique_ptr<IZippyBufferWriter> writer);
        IZippyBufferReader * GetBufferReader();
        IZippyBufferWriter * GetBufferWriter();
        ILogger* GetLogger();
        std::string BuildHTTPResponse(int status, std::string text_info, std::map<std::string, std::string> headers, std::string body);
        HTTPRequest ParseHTTPRequest(const std::string & request_raw);
        void ProcessRequest(HTTPRequest & request);

        private:
        uv_tcp_t * client_connection;
        std::string ip_address;

        std::time_t last_alive_time;
        HTTPRequestHeader current_headers;

        bool hasToWrite{false};
        bool toClose{false};

        std::vector<std::string> send_buffer;
        std::shared_ptr<ILogger> logger;
        std::unique_ptr<IZippyBufferReader> reader;
        std::unique_ptr<IZippyBufferWriter> writer;
        Router & router;

};

class Application{

        public:
        void BindAndListen(int port);

        Router & GetRouter();

        Application();
        ~Application();

        void AddHeaderParser(std::shared_ptr<HTTPHeaderParser> parser);
        std::weak_ptr<HTTPHeaderParser> GetHeaderParsers(const std::string & header_name);
        void SetLogger(std::unique_ptr<ILogger> logger);

        private:
        Router router;

        uv_loop_t * loop;
        uv_tcp_t server;
        
        std::map<std::string, std::shared_ptr<HTTPHeaderParser>> header_parsers;
        std::shared_ptr<ILogger> logger;
};

#endif