/*
* transport.hpp - RAII TCP transport layer.
*
* Exposes a server interface that can accept incomming connections.
* On accept, the server returns a RAII handle to the internal file descriptor.
* It is meant to be used in combination of `framing.hpp` to encapsulate payloads.
*/

#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <unistd.h>

namespace nc {
    class TcpClientHandle {
        public:
            explicit TcpClientHandle(const int fd);
            TcpClientHandle() = default;
            ~TcpClientHandle() {
                if (fd > -1) {
                    close(fd);
                }
            };

            TcpClientHandle(const TcpClientHandle&) = delete;
            TcpClientHandle& operator=(const TcpClientHandle&) = delete;
            TcpClientHandle(TcpClientHandle&&) noexcept;
            TcpClientHandle& operator=(TcpClientHandle&&) noexcept;

            /*
            * Sends the content of buffer.
            *
            * Throws an exception if the socket is disconnected.
            * The value returned - positive or 0 - represents the number
            * of bytes sent from `buffer`.
            */
            int send(const std::span<const uint8_t> buffer);

            /*
            * Sends the content of buffer.
            *
            * Throws an exception if the socket is disconnected.
            * The value returned - positive or 0 - represents the number
            * of bytes received and written to `buffer`.
            */
            int recv(const std::span<uint8_t> buffer);

            int native_handle() const {
                return fd;
            }
        
        private:
            int fd = -1;
    };

    class TcpServer {
        public:
            explicit TcpServer(const uint32_t port);
            ~TcpServer() {
                if (fd > -1) {
                    close(fd);
                }
            };

            TcpServer(const TcpServer&) = delete;
            TcpServer& operator=(const TcpServer&) = delete;
            TcpServer(TcpServer&&) = delete;
            TcpServer& operator=(TcpServer&&) = delete;

            int native_handle() const { return fd; };
            /*
            * Tries to accept a new client.
            *
            * Returns None if no new clients are avaible.
            */
            std::optional<TcpClientHandle> try_accept();
        private:
            int fd;
    };
}
