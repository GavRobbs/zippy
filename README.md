# Zippy

## What is This?

A C++ web application framework with an embedded server. It is single-threaded (for now), non-blocking and supports HTTP 1.1. There is no support for HTTPS yet, but I intend to add it soon. It's not super optimized, because its more for learning purposes, but feel free to fork it and go crazy. The API looks a bit like a mix between Django and Node.js, but in C++.

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

### Current Priorities

- Convert most request headers to a key-value type that supports additional data
- Add a file storage backend option
- Add a templating engine
- Integrate JSON and XML libraries
- Add templated HTML, JSON, XML and file responses