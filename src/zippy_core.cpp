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
#include <mutex>
#include "zippy_core.h"
#include "zippy_utils.h"
#include "networking/socket_buffer_reader.h"
#include "networking/socket_buffer_writer.h"
#include "body_parsers/url_encoded_parser.h"
#include "body_parsers/multipart_parser.h"


Connection::Connection(uv_tcp_t * conn, Router & r, std::shared_ptr<ILogger> logger):client_connection(conn), router(r), logger(logger){
}

Connection::~Connection(){
        if(connection_timer != NULL){
                delete connection_timer;
        }
}

bool Connection::Close(){
        
        /* There were a few interesting bugs I worked through here:
                -Initially, I just called uv_close. The system didn't like that very much and I got a
                uv_close assertion 0 error. Apparently, you can't just close the socket mid transfer
                and expect it to be fine, you have to stop it from reading and ensure its stopped writing.
                - Also uv_close does not fail gracefully if the socket has been already closed.
                - uv_is_closing checks if the socket is already closed, and uv_read_stop forces it to stop reading
                - Writing is a bit more difficult, as there is no explicit write stop, so what I did was have my
                SocketBufferReader class check if toClose is active, and do the closing of the connection from there
                if so.
                - I was still getting segfaults, and then I realized that I never explicitly stopped the timer either,
                because although the connection is dead, the loop is continuing. I changed the code to facilitate that
                and it seems to work.
        */

        toClose = true;

        if(connection_timer != NULL){

                if(uv_is_active((uv_handle_t*)connection_timer)){
                        uv_timer_stop(connection_timer);
                }

        }

        if(!uv_is_closing((uv_handle_t*)client_connection)){


                uv_read_stop((uv_stream_t*)&client_connection);
                if(client_connection->write_queue_size == 0){
                        uv_close((uv_handle_t*)client_connection, NULL);
                        return true;
                } else{
                        return false;
                }
        }

        return true;
}

bool Connection::IsForClosure(){
        return toClose;
}

void Connection::ReadData(){
        reader->ReadData();
}

void Connection::ProcessRequest(HTTPRequest & request){

        RouteFunction func;
        int return_value = 200;

        //Handle form encoding, in the future, will add a dispatcher so this code isn't here
        if(request.header.content_type == "application/x-www-form-urlencoded"){

                URLEncodedParser p;
                return_value = p.Parse(request.body, request.header, request.header.method == "GET" ? request.GET : request.POST, request.FILES);

        } else if(request.header.content_type == "multipart/form-data"){
                MultipartParser p;
                return_value = p.Parse(request.body, request.header, request.header.method == "GET" ? request.GET : request.POST, request.FILES);
        }

        if(return_value != 200){
                std::string data = ZippyUtils::BuildHTTPResponse(return_value, "", {}, "");
                logger->Log(std::string{return_value});
                SendData(data);
                return;
        }

        if(router.FindRoute(request.header.path, func, request.URL_PARAMS)){
                SendData(func(request));
        } else{
                std::string data = ZippyUtils::BuildHTTPResponse(404, "Not found.", {}, "");
                SendData(data);
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

        logger->Log(header_lines[0]);

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
                                StartTimer();
                        } else if(value == "close"){
                                header.keep_alive = false;
                        } else{
                                //Undefined value, should really throw an error here
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

HTTPRequestHeader Connection::GetCurrentHeaders(){

        return this->current_headers;
}

void Connection::SendData(std::string data){

        writer->Write(std::vector<unsigned char>{data.begin(), data.end()});
}

uv_timer_t * Connection::GetConnectionTimer(){
        return connection_timer;
}

void Connection::SetConnectionTimer(uv_timer_t * timer){
        connection_timer = timer;

}

void Connection::StartTimer(){

        if(toClose){
                return;
        }

        connection_timer = new uv_timer_t;
        timer_set = true;
        uv_timer_init(uv_default_loop(), connection_timer);
        connection_timer->data = this;
        uv_timer_start(connection_timer, [](uv_timer_t * handle){

                Connection * c = (Connection*)handle->data;

                if(c != NULL){
                        //c->GetLogger()->Log("Connection closed via timeout");
                        c->Close();
                        delete c;
                }
                

        }, 10000, 0);

}

void Connection::ResetTimer(){

        if(toClose){
                return;
        }

        if(timer_set){

                uv_timer_stop(connection_timer);
                connection_timer->data = this;
                uv_timer_start(connection_timer, [](uv_timer_t * handle){

                        Connection * c = (Connection*)handle->data;

                        if(c != NULL && !c->IsForClosure()){
                                //c->GetLogger()->Log("Connection closed via timeout");
                                c->Close();
                                delete c;
                        }

                }, 10000, 0);

        }

}

void Connection::SetBufferReader(std::unique_ptr<IZippyBufferReader> reader){
        this->reader = std::move(reader);
}

void Connection::SetBufferWriter(std::unique_ptr<IZippyBufferWriter> writer){
        this->writer = std::move(writer);
}

IZippyBufferReader * Connection::GetBufferReader(){
        return this->reader.get();
}

IZippyBufferWriter * Connection::GetBufferWriter(){
        return this->writer.get();
}

ILogger* Connection::GetLogger(){
        return this->logger.get();
}

Connection::Connection(Connection && other) noexcept : router(other.router), client_connection(other.client_connection), 
ip_address(other.ip_address), current_headers(other.current_headers), toClose(other.toClose), 
reader(std::move(other.reader)), writer(std::move(other.writer)), logger(other.logger), send_buffer(other.send_buffer) {

}

Connection & Connection::operator=(Connection && other) noexcept{

        if(this != &other){

                router = other.router;
                client_connection = other.client_connection;
                ip_address = other.ip_address;
                current_headers = other.current_headers;
                toClose = other.toClose;
                reader = std::move(other.reader);
                writer = std::move(other.writer);
                logger = other.logger;
                send_buffer = other.send_buffer;
                connection_timer = other.connection_timer;
        }

        return *this;

}


void Application::BindAndListen(int port){

        loop = uv_default_loop();
        uv_tcp_init(loop, &server);

        sockaddr_in addr;
        uv_ip4_addr("0.0.0.0", port, &addr);
        uv_tcp_bind(&server, (const sockaddr*)&addr, 0);

        server.data = this;

        int rc = uv_listen((uv_stream_t*)&server, 128, [](uv_stream_t * svr, int status){

                Application * app = static_cast<Application*>(svr->data);

                if(status < 0){
                        app->logger->Log("New connection error " + std::string{uv_strerror(status)});
                }

                uv_tcp_t * client = new uv_tcp_t;
                uv_tcp_init(app->loop, client);

                if(uv_accept(svr, (uv_stream_t*)client) == 0){
                        //app->logger->Log("New connection accepted");
                        Connection * c = new Connection{client, app->GetRouter(), app->logger};
                        client->data = (void*)c;
                        c->SetBufferReader(std::make_unique<SocketBufferReader>((uv_stream_t*)client));
                        c->SetBufferWriter(std::make_unique<SocketBufferWriter>((uv_stream_t*)client));
                        c->ReadData();
                } else{
                        uv_close((uv_handle_t*)client, NULL);
                        app->logger->Log("New connection rejected");
                }

        });
        
        if(rc){
                logger->Log("Listen error: " + std::string(uv_strerror(rc)));
        } else{
                logger->Log("Zippy application bound to port " + port);
        }

        uv_run(loop, UV_RUN_DEFAULT);
}

Router & Application::GetRouter()
{
        return router;
}

Application::Application():router(Router{}){

        SetLogger(std::make_unique<NullLogger>());

}

Application::~Application(){

}

void Application::AddHeaderParser(std::shared_ptr<HTTPHeaderParser> parser){

        header_parsers[parser->key_name] = parser;

}

std::weak_ptr<HTTPHeaderParser> Application::GetHeaderParsers(const std::string & header_name){

        std::weak_ptr<HTTPHeaderParser> weakPtr = header_parsers[header_name];
        return weakPtr;

}

void Application::SetLogger(std::unique_ptr<ILogger> logger){
        this->logger = std::move(logger);
}
