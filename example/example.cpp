#include <iostream>
#include <string>

#include <async_http.hpp>

int main() {
	using namespace async_http;

	async_http_request_t req; // Create an instance of the async_http_request_t class

    // Set up HTTP POST request with custom headers and data
    std::string post_url = "http://postman-echo.com/post";
    std::string post_data = R"({"name": "olli", "job": "pro coder"})";

    req.set_header("Authorization", "Bearer 123"); // Set the "Authorization" header
    req.set_header("User-Agent", "AsyncHttp/1.0"); // Set the "User-Agent" header

    // Perform the HTTP POST request with a callback to handle the response
    req.post(post_url, [](const std::string& response) {
        std::cout << "Echo post:\n\n" << response << "\n\n"; // Output the response to the console
    }, content_type_t::json, post_data);

    // Continue processing the request until it is served
    while (!req.serve()) {}

    // Perform an HTTP GET request to fetch data from "http://www.google.com"
    req.get("http://www.google.com", [](const std::string& response) {
        std::cout << "Google get:\n\n" << response; // Output the response to the console
    });

    // Continue processing the request until it is served
    while (!req.serve()) {}

	return 0;
}