#include "flash_sale/http_server.h"

#include <array>
#include <cerrno>
#include <cstring>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace flash_sale {
namespace {

#ifdef _WIN32
using Socket = SOCKET;
constexpr Socket invalid_socket = INVALID_SOCKET;

class SocketRuntime {
public:
    SocketRuntime() {
        WSADATA data{};
        if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
            throw std::runtime_error("WSAStartup failed");
        }
    }

    ~SocketRuntime() { WSACleanup(); }
};

void close_socket(Socket socket) { closesocket(socket); }
int last_socket_error() { return WSAGetLastError(); }
#else
using Socket = int;
constexpr Socket invalid_socket = -1;

class SocketRuntime {};

void close_socket(Socket socket) { close(socket); }
int last_socket_error() { return errno; }
#endif

class SocketHandle {
public:
    explicit SocketHandle(Socket socket = invalid_socket) : socket_(socket) {}
    ~SocketHandle() {
        if (socket_ != invalid_socket) {
            close_socket(socket_);
        }
    }

    SocketHandle(const SocketHandle&) = delete;
    SocketHandle& operator=(const SocketHandle&) = delete;

    Socket get() const { return socket_; }

private:
    Socket socket_;
};

std::string make_response(
    std::string_view status,
    std::string_view body,
    std::string_view content_type = "application/json; charset=utf-8") {
    std::ostringstream response;
    response << "HTTP/1.1 " << status << "\r\n"
             << "Content-Type: " << content_type << "\r\n"
             << "Content-Length: " << body.size() << "\r\n"
             << "Connection: close\r\n"
             << "Cache-Control: no-store\r\n\r\n"
             << body;
    return response.str();
}

std::string route_request(std::string_view request) {
    const auto first_line_end = request.find("\r\n");
    const auto first_line = request.substr(0, first_line_end);

    if (first_line == "GET /health HTTP/1.1" || first_line == "GET /health HTTP/1.0") {
        return make_response("200 OK", R"({"status":"ok","service":"cpp-flash-sale"})");
    }

    if (first_line == "GET / HTTP/1.1" || first_line == "GET / HTTP/1.0") {
        return make_response(
            "200 OK",
            R"({"service":"cpp-flash-sale","version":"0.1.0","health":"/health"})");
    }

    return make_response("404 Not Found", R"({"error":"not_found"})");
}

void send_all(Socket socket, const std::string& response) {
    std::size_t sent = 0;
    while (sent < response.size()) {
        const auto remaining = response.size() - sent;
#ifdef _WIN32
        const int result = send(
            socket,
            response.data() + sent,
            static_cast<int>(remaining),
            0);
#else
        const auto result = send(socket, response.data() + sent, remaining, 0);
#endif
        if (result <= 0) {
            return;
        }
        sent += static_cast<std::size_t>(result);
    }
}

void handle_connection(Socket client) {
    std::array<char, 4096> buffer{};
#ifdef _WIN32
    const int received = recv(client, buffer.data(), static_cast<int>(buffer.size() - 1), 0);
#else
    const auto received = recv(client, buffer.data(), buffer.size() - 1, 0);
#endif
    if (received <= 0) {
        return;
    }

    const std::string_view request(buffer.data(), static_cast<std::size_t>(received));
    send_all(client, route_request(request));
}

}  // namespace

HttpServer::HttpServer(std::uint16_t port) : port_(port) {}

int HttpServer::run() {
    SocketRuntime runtime;
    SocketHandle server(socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
    if (server.get() == invalid_socket) {
        throw std::runtime_error("Failed to create server socket");
    }

    int reuse_address = 1;
#ifdef _WIN32
    setsockopt(
        server.get(),
        SOL_SOCKET,
        SO_REUSEADDR,
        reinterpret_cast<const char*>(&reuse_address),
        sizeof(reuse_address));
#else
    setsockopt(
        server.get(), SOL_SOCKET, SO_REUSEADDR, &reuse_address, sizeof(reuse_address));
#endif

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(port_);

    if (bind(
            server.get(),
            reinterpret_cast<const sockaddr*>(&address),
            sizeof(address)) != 0) {
        throw std::runtime_error(
            "Failed to bind server socket, error=" + std::to_string(last_socket_error()));
    }

    if (listen(server.get(), SOMAXCONN) != 0) {
        throw std::runtime_error(
            "Failed to listen on server socket, error=" + std::to_string(last_socket_error()));
    }

    std::cout << "cpp-flash-sale listening on http://0.0.0.0:" << port_ << '\n';
    std::cout << "health check: http://127.0.0.1:" << port_ << "/health\n";

    while (true) {
        sockaddr_in client_address{};
#ifdef _WIN32
        int client_length = sizeof(client_address);
#else
        socklen_t client_length = sizeof(client_address);
#endif
        SocketHandle client(accept(
            server.get(),
            reinterpret_cast<sockaddr*>(&client_address),
            &client_length));

        if (client.get() == invalid_socket) {
            std::cerr << "Failed to accept connection, error=" << last_socket_error() << '\n';
            continue;
        }

        handle_connection(client.get());
    }
}

}  // namespace flash_sale

