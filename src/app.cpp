/*
 * app.cpp - App life cycle.
 */

#include <iostream>
#include <optional>
#include <poll.h>
#include <string>
#include <utility>

#include "app.hpp"
#include "client_data.hpp"
#include "transport.hpp"
#include "uid.hpp"

namespace nc {
App::App(const std::string &recovery_path, const uint32_t port)
    : server(port), storage(recovery_path) {
    for (struct pollfd &c : client_polls) {
        c.fd = -1;
        c.events = 0;
        c.revents = 0;
    }
    client_polls[0].fd = server.native_handle();
    client_polls[0].events = POLLIN;
}

void App::run() {
    while (true) {
        poll(client_polls, MAX_CLIENTS + 1, -1);

        // Checks server, rare op
        if (client_polls[0].revents != 0) {
            if (std::optional<TcpClientHandle> client = server.try_accept()) {
                this->insert_new_client(std::move(*client));
            }
        }

        for (int i = 0; i < MAX_CLIENTS; ++i) {
            if (client_polls[i + 1].revents != 0) {
                if (std::optional<std::string> str =
                        clients[i].try_recv_next_payload(storage)) {
                    std::cout << "Client " << clients[i].uid() << ": " << *str
                              << std::endl;
                } else {
                    if (!clients[i].is_alive()) {
                        client_polls[i + 1].fd = -1;
                        client_polls[i + 1].events = 0;
                    }
                }
            }
        }
    }
}

void App::insert_new_client(TcpClientHandle client) {
    int fd = client.native_handle();
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (client_polls[i + 1].fd == -1) {
            client_polls[i + 1].fd = fd;
            client_polls[i + 1].events = POLLIN;
            clients[i].init(std::move(client));
            return;
        }
    }
}
} // namespace nc
