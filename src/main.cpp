#include "flash_sale/http_server.h"

#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <limits>
#include <string>

namespace {

std::string read_port_environment() {
#ifdef _WIN32
    char* buffer = nullptr;
    std::size_t size = 0;
    if (_dupenv_s(&buffer, &size, "PORT") != 0 || buffer == nullptr) {
        return {};
    }

    std::string value(buffer);
    std::free(buffer);
    return value;
#else
    const char* value = std::getenv("PORT");
    return value == nullptr ? std::string{} : std::string(value);
#endif
}

std::uint16_t read_port() {
    constexpr std::uint16_t default_port = 8080;
    const std::string value = read_port_environment();
    if (value.empty()) {
        return default_port;
    }

    try {
        const auto parsed = std::stoul(value);
        if (parsed == 0 || parsed > std::numeric_limits<std::uint16_t>::max()) {
            throw std::out_of_range("PORT is outside the valid range");
        }
        return static_cast<std::uint16_t>(parsed);
    } catch (const std::exception& error) {
        std::cerr << "Invalid PORT value '" << value << "': " << error.what() << '\n';
        return default_port;
    }
}

}  // namespace

int main() {
    try {
        flash_sale::HttpServer server(read_port());
        return server.run();
    } catch (const std::exception& error) {
        std::cerr << "Fatal error: " << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
