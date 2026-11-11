/*
* client.cpp - Manages client lifecycle.
*/

#include <arpa/inet.h>
#include <cstring>
#include <span>
#include <stdexcept>
#include <utility>

#include "client.hpp"
#include "framing.hpp"
#include "uid.hpp"

namespace nc {
    void Client::init(TcpClientHandle client) {
        this->reset();
        transport_client_handle = std::move(client);
        _is_alive = true;
    }

    std::optional<std::string> Client::try_recv_next_payload(ClientStorage& client_storage) {
        if (!this->is_alive()) {
            return std::nullopt;
        }

        if (!frame_writer.is_writable()) {
            frame_writer.flush(transport_client_handle);
            return std::nullopt;
        }

        if (this->waiting_handshake()) {
            this->try_handshake(client_storage);
            return std::nullopt;
        }

        uint8_t buffer[MAX_PAYLOAD_SIZE];
        int ret;
        try {
            ret = frame_reader.try_read(transport_client_handle, std::span(buffer, MAX_PAYLOAD_SIZE));
        } catch (const std::runtime_error&) {
            this->reset();
            return std::nullopt;
        }

        if (ret != -1) {
            client_storage.mark_sequence_token_processed(raw_uid);
            return std::string(reinterpret_cast<const char*>(buffer), static_cast<size_t>(ret));
        } else {
            return std::nullopt;
        }
    }

    bool Client::is_alive() const {
       return _is_alive;
    }

    void Client::reset() {
        _is_alive = false;
        frame_reader.reset();
        frame_writer.reset();
        raw_uid = -1;
        transport_client_handle = TcpClientHandle();
    }

    bool Client::waiting_handshake() const {
        return raw_uid == -1;
    }

    int Client::uid() const {
        return raw_uid;
    }

    void Client::try_handshake(ClientStorage& client_storage) {
        uint8_t buffer[4];
        int ret;

        try {
            ret = frame_reader.try_read(transport_client_handle, std::span(buffer, 4));
        } catch (const std::runtime_error&) {
            this->reset();
            return;
        }

        // Still no data received.
        if (ret == -1) {
            return;
        }

        // Received ill-formed data
        if (ret != 4) {
            this->reset();
            return;
        }

        uint32_t client_id;
        std::memcpy(&client_id, buffer, 4);
        client_id = ntohl(client_id);

        if (client_id < 0 || client_id >= MAX_CLIENTS) {
            this->reset();
            return;
        }

        raw_uid = client_id;

        // Reply with next sequence number
        uint32_t sequence_number = client_storage.next_sequence_token(client_id);
        sequence_number = htonl(sequence_number);
        std::memcpy(buffer, &sequence_number, 4);
        try {
            ret = frame_writer.try_write(transport_client_handle, std::span(buffer, 4));
        } catch (const std::runtime_error&) {
            this->reset();
            return;
        }
    }
}
