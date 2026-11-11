#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <string>

#include "app.hpp"

int main() {
    std::signal(SIGPIPE, SIG_IGN);

    const char* path_env = std::getenv("STORAGE_PATH");
    std::string recovery_path = path_env ? path_env : "clients.dat";

    const char* port_env = std::getenv("PORT");
    uint32_t port = port_env ? static_cast<uint32_t>(std::stoul(port_env)) : 8080;

    nc::App app(recovery_path, port);

    app.run();

    return 0;
}
