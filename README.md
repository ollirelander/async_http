# HTTP GET/POST Single-Header C++17 Library [![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)

This single-header C++17 library provides a modern, simple, and fast solution for performing HTTP GET and POST requests in your C++ projects. It is designed to streamline the process of interacting with web servers and APIs, making it a go-to choice for developers seeking an efficient HTTP communication solution.

## Installation

To use this library, simply include the `async_http.hpp` header file in your project. Since it's a single-header library, there are no external dependencies to set up.

## Usage

Here's an example demonstrating how to use the library to perform HTTP GET and POST requests:

```c++
#include <iostream>
#include "async_http.hpp"

int main() {
    using namespace async_http;

    async_http_request_t req;

    // HTTP POST request example
    std::string post_url = "http://postman-echo.com/post";
    std::string post_data = R"({"name": "olli", "job": "pro coder"})";

    req.set_header("Authorization", "Bearer 123");
    req.set_header("User-Agent", "AsyncHttp/1.0");

    req.post(post_url, [](const std::string& response) {
        std::cout << "Echo post:\n\n" << response << "\n\n";
    }, content_type_t::json, post_data);

    while (!req.serve()) {}

    // HTTP GET request example
    req.get("http://www.google.com", [](const std::string& response) {
        std::cout << "Google get:\n\n" << response;
    });

    while (!req.serve()) {}

    return 0;
}
```

### Customizable Options

The library offers flexibility and customization through various options. You can set custom headers, specify the request's content type, and provide data for HTTP POST requests according to your specific needs.

### Asynchronous Handling

The library utilizes an asynchronous approach, allowing you to perform multiple HTTP requests concurrently without blocking the main thread. The `serve()` method ensures that requests are processed efficiently, avoiding unnecessary delays.

### Modern C++17

Built with modern C++17 standards, this library ensures compatibility with contemporary projects, taking advantage of the latest language features and improvements.

## Performance

With its lightweight design and efficient implementation, this library achieves impressive performance, delivering fast HTTP request processing for your applications.

## Simplicity

Integrating this library into your projects is straightforward. Its clean and intuitive API minimizes boilerplate code and simplifies the process of making HTTP requests.

Whether you need to fetch data, interact with RESTful APIs, or submit forms, this single-header C++17 library offers a powerful and user-friendly solution for HTTP communication.

Give it a try and experience the modern, simple, and fast HTTP GET/POST functionality in your C++ projects!