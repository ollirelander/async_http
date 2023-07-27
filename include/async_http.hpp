#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h> 
#include <unistd.h>

using SOCKET = int;
#endif

#include <functional>
#include <chrono>
#include <string_view>
#include <vector>
#include <string>
#include <cstdint>

namespace async_http {
	namespace impl {
		bool initialize() noexcept;
		bool deinitialize() noexcept;

		static class initializer_t {
		public:
			initializer_t() noexcept {
				initialize();
			}
			~initializer_t() noexcept {
				deinitialize();
			}
		} initializer;

		class tcp_socket_t {
		public:
			tcp_socket_t() noexcept;
			~tcp_socket_t() noexcept;

			bool connect(const std::string& host, std::uint16_t port) noexcept;
			bool close() noexcept;

			bool send(const std::byte* data, std::size_t len) noexcept;
			bool receive(std::byte* data, std::size_t len) noexcept;
		private:
			SOCKET raw_socket;
		};
#ifdef _WIN32
		bool initialize() noexcept {
			WSADATA wsa_data;
			return !(WSAStartup(MAKEWORD(1, 1), &wsa_data));
		}

		bool deinitialize() noexcept {
			return !(WSACleanup());
		}

		tcp_socket_t::tcp_socket_t() noexcept : raw_socket(-1) {}

		tcp_socket_t::~tcp_socket_t() noexcept {
			close();
		}

		bool tcp_socket_t::connect(const std::string& host, std::uint16_t port) noexcept {
			raw_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (raw_socket == INVALID_SOCKET) {
				return false;
			}

			if (u_long mode = 1; ioctlsocket(raw_socket, FIONBIO, &mode) == SOCKET_ERROR) {
				return false;
			}

			addrinfo hints = {}, * res;
			hints.ai_family = AF_INET;
			hints.ai_socktype = SOCK_STREAM;

			int result = getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res);
			if (result != 0) {
				return false;
			}

			result = ::connect(raw_socket, res->ai_addr, res->ai_addrlen);
			freeaddrinfo(res);

			if (result == SOCKET_ERROR) {
				if (WSAGetLastError() == WSAEWOULDBLOCK) {
					return true;
				}
				return false;
			}

			return true;
		}

		bool tcp_socket_t::close() noexcept {
			if (raw_socket != INVALID_SOCKET && closesocket(raw_socket) != SOCKET_ERROR) {
				return true;
			}
			return false;
		}

		bool tcp_socket_t::send(const std::byte* data, std::size_t len) noexcept {
			if (::send(raw_socket, reinterpret_cast<const char*>(data), len, 0) != SOCKET_ERROR) {
				return true;
			}

			return false;
		}

		bool tcp_socket_t::receive(std::byte* data, std::size_t len) noexcept {
			int bytes_received = recv(raw_socket, reinterpret_cast<char*>(data), len, 0);

			if (bytes_received != SOCKET_ERROR && bytes_received != 0) {
				return true;
			}
			else {
				return false;
			}
		}
#else
		bool initialize() noexcept {
			return true;
		}

		bool deinitialize() noexcept {
			return true;
		}

		tcp_socket_t::tcp_socket_t() noexcept : raw_socket(-1) {}

		tcp_socket_t::~tcp_socket_t() noexcept {
			close();
		}

		bool tcp_socket_t::connect(const std::string& host, std::uint16_t port) noexcept {
			raw_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (raw_socket == -1) {
				return false;
			}

			int flags = fcntl(raw_socket, F_GETFL, 0);
			if (flags == -1) {
				return false;
			}

			if (fcntl(raw_socket, F_SETFL, flags | O_NONBLOCK) == -1) {
				return false;
			}

			addrinfo hints = {}, * res;
			hints.ai_family = AF_INET;
			hints.ai_socktype = SOCK_STREAM;

			int result = getaddrinfo(host.c_str(), std::to_string(port).c_str(), &hints, &res);
			if (result != 0) {
				return false;
			}

			result = ::connect(raw_socket, res->ai_addr, res->ai_addrlen);
			freeaddrinfo(res);

			if (result == -1) {
				if (errno == EINPROGRESS) {
					return true;
				}
				return false;
			}

			return true;
		}

		bool tcp_socket_t::close() noexcept {
			if (raw_socket != -1 && ::close(raw_socket) == 0) {
				return true;
			}
			return false;
		}

		bool tcp_socket_t::send(const std::byte* data, std::size_t len) noexcept {
			if (check_socket_state(tcp_socket_state::write) && !check_socket_state(tcp_socket_state::error)) {
				if (::send(raw_socket, reinterpret_cast<const char*>(data), len, 0) != -1) {
					return true;
				}
			}

			return false;
		}

		bool tcp_socket_t::receive(std::byte* data, std::size_t len) noexcept {
			if (check_socket_state(tcp_socket_state::read) && !check_socket_state(tcp_socket_state::error)) {
				int bytes_received = recv(raw_socket, reinterpret_cast<char*>(data), len, 0);
				if (bytes_received != -1 && bytes_received != 0) {
					return true;
				}
			}

			return false;
		}
#endif
	}

	enum class content_type_t {
		form_urlencoded,
		json,
		xml,
		text
	};

	class async_http_request_t {
	public:
		using callback_t = std::function<void(const std::string& response)>;

		async_http_request_t() noexcept : response_buffer(4096), ready_to_receive(false) {}

		std::string get_header(const std::string& name) const noexcept {
			auto it = headers.find(name);
			if (it != headers.end()) {
				return it->second;
			}
			return "";
		}

		void set_header(const std::string& name, const std::string& value) noexcept {
			headers[name] = value;
		}

		void remove_header(const std::string& name) noexcept {
			headers.erase(name);
		}

		void clear_headers() noexcept {
			headers.clear();
		}

		void get(std::string_view url, const callback_t& callback) noexcept {
			std::string host;
			std::string path;

			if (!parse_url(url, host, path)) {
				callback("Invalid URL");
				return;
			}

			if (!socket.connect(host, 80)) {
				callback("Connection failed");
				return;
			}

			request = "GET " + path + " HTTP/1.1\r\n"
				+ "Host: " + host + "\r\n";

			for (const auto& header : headers) {
				request += header.first + ": " + header.second + "\r\n";
			}

			request += "Connection: close\r\n\r\n";

			response.clear();

			ready_to_receive = false;
			this->callback = callback;
		}

		void post(std::string_view url, callback_t callback, content_type_t content_type, const std::string& data) noexcept {
			std::string host;
			std::string path;

			if (!parse_url(url, host, path)) {
				callback("Invalid URL");
				return;
			}

			if (!socket.connect(host, 80)) {
				callback("Connection failed");
				return;
			}

			std::string content_type_str;
			switch (content_type) {
			case content_type_t::form_urlencoded:
				content_type_str = "application/x-www-form-urlencoded";
				break;
			case content_type_t::json:
				content_type_str = "application/json";
				break;
			case content_type_t::xml:
				content_type_str = "application/xml";
				break;
			case content_type_t::text:
				content_type_str = "text/plain";
				break;
			default:
				content_type_str = "application/octet-stream";
				break;
			}

			std::string encoded_data = (content_type == content_type_t::form_urlencoded) ? url_encode(data) : data;

			request = "POST " + path + " HTTP/1.1\r\n"
				+ "Host: " + host + "\r\n"
				+ "Content-Type: " + content_type_str + "\r\n"
				+ "Content-Length: " + std::to_string(encoded_data.length()) + "\r\n";

			for (const auto& header : headers) {
				request += header.first + ": " + header.second + "\r\n";
			}

			request += "Connection: close\r\n\r\n";
			request += encoded_data;

			response.clear();

			ready_to_receive = false;
			this->callback = callback;
		}

		bool serve() noexcept {
			if (ready_to_receive) {
				if (socket.receive(response_buffer.data(), response_buffer.size())) {
					response.append(reinterpret_cast<const char*>(response_buffer.data()));
					response_buffer.assign(response_buffer.size(), std::byte{ 0 });

					packet_timeout = std::chrono::system_clock::now();
					packet_timeout += std::chrono::milliseconds(50);
				}

				if (std::chrono::system_clock::now() >= packet_timeout) {
					callback((response.empty()) ? "No data" : response);
					socket.close();
					return true;
				}
			}
			else if (socket.send(reinterpret_cast<const std::byte*>(request.c_str()), request.size())) {
					packet_timeout = std::chrono::system_clock::now();
					packet_timeout += std::chrono::milliseconds(5000);

					ready_to_receive = true;
			}

			return false;
		}
	private:
		bool parse_url(std::string_view url, std::string& host, std::string& path) const noexcept {
			size_t protocol_pos = url.find("://");
			if (protocol_pos == std::string::npos) {
				return false;
			}

			protocol_pos += 3;
			size_t host_end = url.find('/', protocol_pos);
			if (host_end == std::string::npos) {
				host = url.substr(protocol_pos);
				path = "/";
			}
			else {
				host = url.substr(protocol_pos, host_end - protocol_pos);
				path = url.substr(host_end);
			}

			return true;
		}

		std::string url_encode(const std::string& data) const noexcept {
			std::string encoded_data;
			const std::string safe_chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-_.~";
			for (char c : data) {
				if (std::isalnum(c) || safe_chars.find(c) != std::string::npos) {
					encoded_data.push_back(c);
				}
				else {
					encoded_data += "%" + to_hex(static_cast<unsigned char>(c));
				}
			}
			return encoded_data;
		}

		std::string to_hex(unsigned char c) const noexcept {
			const char* hex_digits = "0123456789ABCDEF";
			std::string result;

			result.push_back(hex_digits[c >> 4]);
			result.push_back(hex_digits[c & 15]);

			return result;
		}

		impl::tcp_socket_t socket;

		std::chrono::time_point<std::chrono::system_clock> packet_timeout;

		std::vector<std::byte> response_buffer;

		std::string request;
		std::string response;

		std::unordered_map<std::string, std::string> headers;

		bool ready_to_receive;

		callback_t callback;
	};
}
