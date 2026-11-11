/*
* transport.cpp - RAII TCP transport layer.
*/

#include <arpa/inet.h>
#include <fcntl.h>
#include <stdexcept>
#include <sys/socket.h>

#include "transport.hpp"

namespace nc {
    inline constexpr int BACKLOG = 3;

    static void set_nonblocking(int fd) {
        if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1) {
            throw std::runtime_error(std::string("fcntl F_SETFL failed: ") + std::strerror(errno));
        }
    }

    /*
    * ----------------- TcpClientHandle impl -----------------
    */

    TcpClientHandle::TcpClientHandle(const int fd): fd(fd) {
        set_nonblocking(fd);
    }

    int TcpClientHandle::send(const std::span<const uint8_t> buffer) {
        if (fd == -1) {
            throw std::runtime_error(std::string("invalid handle"));
        }

        const ssize_t ret = ::send(fd, buffer.data(), buffer.size(), 0);

        if (ret == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                return 0;
            } else {
                throw std::runtime_error(std::string("client socket error: ") + std::strerror(errno));
            }
        }

        return ret;
    }

    int TcpClientHandle::recv(const std::span<uint8_t> buffer) {
        if (fd == -1) {
            throw std::runtime_error(std::string("invalid handle"));
        }

        const ssize_t ret = ::recv(fd, buffer.data(), buffer.size(), 0);

        if (ret == 0) {
            throw std::runtime_error(std::string("client disconnected"));
        }

        if (ret == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                return 0;
            } else {
                throw std::runtime_error(std::string("client socket error: ") + std::strerror(errno));
            }
        }

        return ret;
    }

    TcpClientHandle::TcpClientHandle(TcpClientHandle&& other) noexcept
        : fd(other.fd) {
        other.fd = -1;
    }

    TcpClientHandle& TcpClientHandle::operator=(TcpClientHandle&& other) noexcept {
        if (this != &other) {
            if (fd > -1) {
                close(fd);
            }
            fd = other.fd;
            other.fd = -1;
        }
        return *this;
    }

    /*
    * ----------------- TcpServer impl -----------------
    */

    TcpServer::TcpServer(const uint32_t port) {
        fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd == -1) {
            throw std::runtime_error(std::string("unable to create server socket: ") + strerror(errno));
        }

        set_nonblocking(fd);

        struct sockaddr_in address {
            .sin_family=AF_INET,
            .sin_addr.s_addr=INADDR_ANY,
            .sin_port=htons(port),
        };

        const int bind_ret = bind(fd, reinterpret_cast<struct sockaddr*>(&address), sizeof(address));
        if (bind_ret == -1) {
            throw std::runtime_error(std::string("unable to bind server socket: ") + strerror(errno));
        }

        const int listen_ret = listen(fd, BACKLOG);
        if (listen_ret == -1) {
            throw std::runtime_error(std::string("unable to listen to server socket: ") + strerror(errno));
        }
    }

    std::optional<TcpClientHandle> TcpServer::try_accept() {
        struct sockaddr_in peer_address;
        socklen_t peer_addr_size = sizeof(peer_address);
        const int client_fd = ::accept(fd, reinterpret_cast<struct sockaddr*>(&peer_address), &peer_addr_size);
        if (client_fd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                return std::nullopt;
            }
            throw std::runtime_error(std::string("unable to accept client: ") + strerror(errno));
        }
        
        return std::optional<TcpClientHandle>(std::in_place, client_fd);
    }
}
