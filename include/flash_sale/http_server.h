#pragma once

#include <cstdint>

namespace flash_sale {

class HttpServer {
public:
    explicit HttpServer(std::uint16_t port);

    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;

    int run();

private:
    std::uint16_t port_;
};

}  // namespace flash_sale

