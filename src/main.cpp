#include <iostream>
#include "zippy_core.h"

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
</body>
</html>)";

int main(int argc, char **argv){

        Application app{};
        app.Bind(3500);

        app.GetRouter().AddRoute("/", [](const HTTPRequest & request) -> std::string {

                return ZippyUtils::BuildHTTPResponse(200, "OK", {}, webpage_body);

        });

        app.GetRouter().AddRoute("/poetry", [](const HTTPRequest & request) -> std::string {

                return ZippyUtils::BuildHTTPResponse(200, "OK", {}, poetry_body);

        });

        app.GetRouter().AddRoute("/poetry/another", [](const HTTPRequest & request) -> std::string {

                return ZippyUtils::BuildHTTPResponse(200, "OK", {}, poetry2_body);

        });
        
        while(true){

                app.Listen();

        }
        return 0;
}

