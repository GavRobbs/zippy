# Zippy

## What is This?

A C++ web application framework with an embedded server for Linux. It uses a non-blocking event loop and supports HTTP 1.1. It's not super optimized, because its more for learning purposes, but feel free to fork it and optimize away. The API looks a bit like a mix between Django and Node.js, but in C++.

## Why?

I built this to teach myself more about networking and the HTTP standard. Also, I hope to one day re-deploy my blog on it.

## How?

Built using C++ 17 on an old Raspberry Pi 3B+ with dead GPIO pins.

## When?

When it's done.

### Changelog

- 16/8/24: Made the server non-blocking, with proper Content-Length serving.
- 16/8/24: Fixed the handling of query parameters and added support for the Content-Length header
- 17/8/24: Separated the zippy library from the demo
- 17/8/24: Fixed the trie implementation and added unit tests for it
- 17/8/24: Cleaned up the root CMakeLists file a bit and added two options
- 17/8/24: Set up CI with Github Actions
- 18/8/24: Made the main loop more efficient by fixing the allocation of the Connection object, also modified the API a bit
- 18/8/24: Added an interface for buffer reads and created the SocketBufferReader class which is automatically added to the connection class
- 18/8/24: Created an include file for the exceptions used by the library
- 19/8/24: Converted the library to a dynamic library and added the beginnings of an extension system using dynamic libraries
- 20/8/24: Converted the library to a multithreaded connection system
- 21/8/24: Rewrote the console logger and file logger extensions to be thread safe
- 29/9/24: Rewrote the core connection code code to use the libuv event loop
- 30/9/24: Fixed some memory leakage and the overall stability of the core connection code
- 30/9/24: Added date and time to the loggers
- 30/9/24: Removed the threads requirement from the CMakeLists.txt file
- 8/10/24: Added an IZippyBodyParser interface and added a parsers to handle URL encoded data and multipart data. Updated the example accordingly.
- 8/10/24: Fixed some errors with the buffer reader.
- 11/4/24: Started writing my notes. Wrote some of the information I got about HTTP.

### Current Priorities

- MIME type checking for multipart uploaded data
- Add a file storage backend option
- Convert most request headers to a key-value type that supports additional data
- Add a templating engine
- Integrate JSON and XML libraries
- Add templated HTML, JSON, XML and file responses