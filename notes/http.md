# HTTP

## What is HTTP?

HTTP stands for Hypertext Transfer Protocol. It defines a method by which resources are accessed over the internet.

Hypertext is a software system that allows for extensive crosslinking between text and images - think like jumping around in an old choose your own adventure book.

HTTP is a request-response protocol. A request originates from the client and is sent to the server, which in turn, sends a response. A request can be:
- Asking for a resource stored on the server
- Adding a resource to the server
- Updating or deleting a resource stored on the server

The response contains the requested resource or information about the success/failure of the requested operation on the resource.

HTTP is an application layer protocol and is sent over TCP (which for HTTPS is encrypted).

## HTTP Requests and Responses

An example of a HTTP request is as follows:

```
GET /path/to/resource HTTP/1.1
Host: www.example.com
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64)
Accept: text/html
Connection: close

```

The first line identifies this as a HTTP request and contains important information. It always consists of 3 parts: the verb, the resource path and the HTTP version. The verb specifies the action to be done on the resource and can be GET, POST, PUT, PATCH etc. The path specifies where on the server we want to look for the resource. The HTTP version indicates what version of the HTTP protocol we are using, which is important, because there are some differences in the defaults between different HTTP versions.

After the first line, there are zero or more headers, which give additional information to the server about how we want it to handle the request. The headers are represented as key-value pairs (eg. Host: www.example.com), with the key being separated from the value by a colon and each pair going on its own line. HTTP headers are supposed to be case insensitive, so "host" is the same as "Host" is the same as "HOST". In practice however, headers are often written in Title-Case for readability.

The structure of a newline in requests/responses is also important, as the HTTP/1.1 specification says that lines must end with \r\n (ie. CR+LF), ending with \n alone is not compliant with the standard and should lead to undefined behaviour. This is something to keep in mind when constructing your own requests by hand or parsing responses.

A HTTP response looks like the following:

```
HTTP/1.1 200 OK
Date: Sun, 03 Nov 2024 17:00:00 GMT
Server: Apache/2.4.41 (Ubuntu)
Content-Type: text/html; charset=UTF-8
Content-Length: 137
Connection: close

<!DOCTYPE html>
<html>
<head>
    <title>Sample Response</title>
</head>
<body>
    <h1>Hello, world!</h1>
    <p>This is a sample HTTP response.</p>
</body>
</html>
```

The first line identifies the data as a HTTP response and consists of a HTTP version, a status code and a reason phrase, all separated by a space. There are dozens of different status codes, although maybe only 20 or so are in common use. They can be used to indicate the success or failure of the triggering request or that the request is incomplete. The reason phrase is optional and can be omitted in HTTP/1.1.

HTTP requests and responses have two parts: the header and the body. The header contains the first line and all the key-value pairs that specify headers after. It is separated from the body, which contains the content we want to send or the content we receive by an empty line, ie:

HEADER
BLANK LINE
BODY

The blank line is specified by a CR+LF. Practically speaking, this means we can jump to the start of the body by finding the first instance of '\r\n\r\n' (the first \r\n corresponds to the last line of the key-value pairs, and the second one corresponds to the blank line itself), then adding 4 to skip over those characters.

## HTTP Versions

- HTTP 0.9: This was the earliest verion, designed for simple retrieval of documents on the web. It didn't support headers.
- HTTP 1.0: This version added support for headers, additional verbs (eg. POST) and status codes.
- HTTP 1.1: This added extra verbs and most importantly, made persistent connections the default, allowing a single TCP connection to be used for multiple requests/responses, improving performance.
- HTTP 2&3: Out of the scope for this project, but these are binary protocols that are no longer human-readable.

## Reading HTTP Requests

Zippy is built on top of libuv, allowing me to use a non-blocking event loop model. 

One problem that I faced was knowing when I had read the entirety of a HTTP request. I had to be sure I read all of the header, as well as all of the body. Luckily, thanks to how the HTTP headers are defined, I could determine if I read the entire header by seeing if the '\r\n\r\n' sequence was present. 

This left the problem of the body. This problem was solved definitionally, because by the HTTP/1.0 standard all messages (ie. requests/responses) with a body must have a Content-Length header, unless some other way of specifying the message body is included (eg. chunked encoding). If neither is present, the connection is expected to close after the header is read, regardless of how much of the body has been parsed.

In my code, I handled this in the SocketBufferReader class. The ReadData method encapsulates allocating a buffer and reading bytes from the socket. nread indicates the number of bytes read, and this is appended to the raw_body member variable. If we determine that the entire header has been read, then we look for Content-Length or chunked transfer encoding. From here (possibly over multiple passes), we read Content-Length number of bytes or read the chunks until we reach the special ending chunk. When reading is complete, we then look at the Content-Type to dispatch and process it appropriately.