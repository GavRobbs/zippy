#include <iostream>
#include <sstream>
#include <memory>
#include "zippy_core.h"
#include "extensions/extension_factory.h"

std::string webpage_header = "HTTP/1.1 200 OK\r\n\r\n";
std::string webpage_body = R"(
<html>
<head>
        <title>A directly served webpage</title>
</head>
<body>
        <h1>Gavin's Personal Blog </h1>
        <p>Straight from the trenches! This is currently running on Ubuntu 22 on a Raspberry Pi 3b+ with fried GPIO pins.</p>
        <p>Click <a href="/poetry">here</a> to see some poetry. </p>
        <form action="/upload" method="POST" enctype="multipart/form-data">
                <p>Enter your information here</p>
                <label for="yourName">Your Name</label><br />
                <input type="text" name="name" id="yourName"><br/>
                <label for="yourCountry">Your Country</label><br />
                <input type="text" name="home_country" id="yourCountry">
                <label for="yourPic">Your Picture</label><br />
                <input type="file" name="photo" id="yourPic">
                <input type="submit">
        </form>
</body>
</html>)";

std::string form_submit_body = R"(<html>
<head>
        <title>A directly served webpage</title>
</head>
<body>
        <h1>Thank you for submitting</h1>
        <p>Your input has been considered!</p>
        <p>Click <a href="/">here</a> to go home. </p>
</body>
</html>)";

std::string poetry_body = R"(
<html>
<head>
        <title>Gavin's Blog - Poetry</title>
</head>
<body>
There was a young girl from Madras, <br />
Who had an adorable ass. <br />
Not rounded and pink <br />
As you probably think — <br />
It was grey, had long ears, and ate grass. <br />
<p>Click <a href="/poetry/another">here</a> for another. </p>
</body>
</html>)";

std::string poetry2_body = R"(
<html>
<head>
        <title>Gavin's Blog - Poetry</title>
</head>
<body>
There was a young sailor from Brighton, <br />
Who said to his girl, "You're a tight one." <br />
She replied, "Bless my soul, <br />
You're in the wrong hole; <br />
There's plenty of room in the right one." <br />
<p>Click <a href="/">here</a> to go home. </p>
</body>
</html>)";

std::string json_response = R"({
\"status\": \"created\"
}
)";

int main(int argc, char **argv){

        Application app{};

        ZippyExtensionFactory extension_factory{};
        extension_factory.LoadExtensionsFromFolder("./extensions/");
        ILogger * logger = extension_factory.CreateLogger("ConsoleLogger");

        app.SetLogger(std::unique_ptr<ILogger>(logger));

        app.GetRouter().AddRoute("/", [](const HTTPRequest & request) -> std::string {

                return ZippyUtils::BuildHTTPResponse(200, "OK", {}, webpage_body);

        });

        app.GetRouter().AddRoute("/poetry", [](const HTTPRequest & request) -> std::string {

                return ZippyUtils::BuildHTTPResponse(200, "OK", {}, poetry_body);

        });

        app.GetRouter().AddRoute("/poetry/another", [](const HTTPRequest & request) -> std::string {

                return ZippyUtils::BuildHTTPResponse(200, "OK", {}, poetry2_body);

        });

        app.GetRouter().AddRoute("/upload", [](HTTPRequest & request) -> std::string {

                if(request.header.method == "POST"){
                        std::cout << (std::string)request.POST["name"] << std::endl;
                        std::cout << (std::string)request.POST["home_country"] << std::endl;

                        return ZippyUtils::BuildHTTPResponse(201, "OK", {
                                { "Content-Type", "text/html" },
                        }, form_submit_body);
                } else{
                        return ZippyUtils::BuildHTTPResponse(404, "Not Found.", {}, "");
                }

        });

        app.GetRouter().AddRoute("/test_qp", [](HTTPRequest & request) -> std::string {

                std::stringstream ss;
                ss << "{" << std::endl;

                //Can replace this with a nicer example when I add the json processing library

                for(auto it = request.GET.begin(); it != request.GET.end(); ++it){
                        ss << "\"" << it->first << "\" : \"" << static_cast<std::string>(it->second) << "\"," << std::endl;
                }

                ss << "}" << std::endl;

                return ZippyUtils::BuildHTTPResponse(201, "OK", {
                                { "Content-Type", "application/json" }
                        }, ss.str());

        });

        app.BindAndListen(3500);

        
        return 0;
}

