/*
 * main_client.cpp - Minimal TCP client using nc::TcpClientHandle and
 * nc::FrameWriter/FrameReader.
 */

#include <arpa/inet.h>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <span>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

#include "framing.hpp"
#include "transport.hpp"

namespace {
nc::TcpClientHandle connect_to_server(const std::string &host, uint16_t port) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        throw std::runtime_error(std::string("socket failed: ") +
                                 std::strerror(errno));
    }

    sockaddr_in addr{
        .sin_family = AF_INET,
        .sin_port = htons(port),
    };
    if (::inet_pton(AF_INET, host.c_str(), &addr.sin_addr) != 1) {
        ::close(fd);
        throw std::runtime_error("invalid host address: " + host);
    }

    if (::connect(fd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) ==
        -1) {
        ::close(fd);
        throw std::runtime_error(std::string("connect failed: ") +
                                 std::strerror(errno));
    }

    return nc::TcpClientHandle(fd);
}

void write_frame_blocking(nc::FrameWriter &writer, nc::TcpClientHandle &handle,
                          std::span<const uint8_t> payload) {
    if (writer.try_write(handle, payload)) {
        return;
    }
    while (!writer.flush(handle)) {
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
}

int read_frame_blocking(nc::FrameReader &reader, nc::TcpClientHandle &handle,
                        std::span<uint8_t> out) {
    while (true) {
        int n = reader.try_read(handle, out);
        if (n != -1) {
            return n;
        }
        std::this_thread::sleep_for(std::chrono::microseconds(1));
    }
}

uint32_t handshake(nc::TcpClientHandle &handle, nc::FrameWriter &writer,
                   nc::FrameReader &reader, uint32_t service_id) {
    uint8_t buf[sizeof(uint32_t)];
    uint32_t net_id = htonl(service_id);
    std::memcpy(buf, &net_id, sizeof(net_id));
    write_frame_blocking(writer, handle, buf);

    int len = read_frame_blocking(reader, handle, buf);
    if (len != sizeof(uint32_t)) {
        throw std::runtime_error("unexpected handshake reply size");
    }

    uint32_t net_seq = 0;
    std::memcpy(&net_seq, buf, sizeof(net_seq));
    return ntohl(net_seq);
}
} // namespace

int main() {
    std::signal(SIGPIPE, SIG_IGN);

    const char *service_env = std::getenv("SERVICE_ID");
    if (!service_env) {
        std::cerr << "SERVICE_ID environment variable is required\n";
        return 1;
    }
    const uint32_t service_id = static_cast<uint32_t>(std::stoul(service_env));

    const std::string host = "127.0.0.1";

    const char *port_env = std::getenv("PORT");
    const uint16_t port =
        port_env ? static_cast<uint16_t>(std::stoul(port_env)) : 8080;

    try {
        nc::TcpClientHandle handle = connect_to_server(host, port);
        nc::FrameWriter writer;
        nc::FrameReader reader;

        uint32_t seq = handshake(handle, writer, reader, service_id);
        std::cout << "Connected (service_id=" << service_id
                  << "), starting at sequence " << seq << std::endl;

        while (true) {
            std::string msg = "sequence: " + std::to_string(seq);
            std::span<const uint8_t> bytes(
                reinterpret_cast<const uint8_t *>(msg.data()), msg.size());
            write_frame_blocking(writer, handle, bytes);
            std::cout << "Sent: " << msg << std::endl;
            ++seq;
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    } catch (const std::exception &e) {
        std::cerr << "Client error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
