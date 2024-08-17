# Zippy

## What is This?

A C++ web application framework with an embedded server. It is single-threaded (for now), non-blocking and supports HTTP 1.1. There is no support for HTTPS yet, but I intend to add it soon.

## Why?

I built this to teach myself more about networking and the HTTP standard. Also, I hope to one day re-deploy my blog on it.

## How?

Built using C++ 17 on an old Raspberry Pi 3B+ with dead GPIO pins.

## When?

When it's done.

### Changelog

16/8/24: Made the server non-blocking, with proper Content-Length serving.
16/8/24: Fixed the handling of query parameters and added support for the Content-Length header