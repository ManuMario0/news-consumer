/*
* framing.cpp - Framing over TCP socket.
*/

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <sys/_endian.h>
#include <unistd.h>

#include "framing.hpp"

namespace nc {
    uint32_t host_to_network_size(size_t size) {
        uint32_t cast_size = static_cast<uint32_t>(size);
        return htonl(cast_size);
    }

    size_t network_to_host_size(uint32_t size) {
        uint32_t host_size = ntohl(size);
        return static_cast<size_t>(host_size);
    }

    /*
    * ----------------- FrameWriter impl -----------------
    */


    bool check_size(std::span<const uint8_t> payload) {
        return payload.size() <= MAX_PAYLOAD_SIZE;
    }

    bool FrameWriter::try_write(int fd, std::span<const uint8_t> payload) {
        if (!this->is_writable()) {
            return false;
        }

        if (!check_size(payload)) {
            throw std::runtime_error(std::string("payload is too big for transport"));
        }

        size_t payload_size = payload.size();
        uint32_t n_payload_size = host_to_network_size(payload_size);

        std::memcpy(buffer, &n_payload_size, HEADER_SIZE);
        std::memcpy(&buffer[HEADER_SIZE], payload.data(), payload_size);

        written = 0;
        frame_size = HEADER_SIZE + payload_size;

        return this->flush(fd);
    }

    bool FrameWriter::is_writable() const {
        return this->bytes_left_to_write() == 0;
    }

    bool FrameWriter::flush(int fd) {
        if (this->is_writable()) {
            return true;
        }

        ssize_t fresh_written_bytes = write(fd, &buffer[written], this->bytes_left_to_write());

        if (fresh_written_bytes == 0) {
            throw std::runtime_error(std::string("socket disconnected"));
        }
        
        if (fresh_written_bytes < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                return false;
            } else {
                throw std::runtime_error(std::string("socket error") + std::strerror(errno));
            }
        }

        written += fresh_written_bytes;

        return this->is_writable();
    }

    size_t FrameWriter::bytes_left_to_write() const {
        return frame_size - written;
    }


    /*
    * ----------------- FrameReader impl -----------------
    */

    bool FrameReader::reading_header() const {
        return frame_size == 0;
    }

    bool FrameReader::try_read_header(int fd) {
        if (!FrameReader::reading_header()) {
            return true;
        }

        size_t fresh_read_bytes = read(fd, &buffer[read_bytes], HEADER_SIZE - read_bytes);

        if (fresh_read_bytes == 0) {
            throw std::runtime_error(std::string("socket disconnected"));
        }

        if (fresh_read_bytes < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                return false;
            } else {
                throw std::runtime_error(std::string("socket error") + std::strerror(errno));
            }
        }

        read_bytes += fresh_read_bytes;

        if (read_bytes == HEADER_SIZE) {
            uint32_t header;
            std::memcpy(&header, buffer, HEADER_SIZE);
            size_t payload_size = network_to_host_size(header);

            if (payload_size > MAX_PAYLOAD_SIZE) {
                throw std::runtime_error(std::string("received corrupted payload"));
            }

            frame_size = HEADER_SIZE + payload_size;

            return true;
        }

        return false;
    }
    
    bool FrameReader::is_empty_payload() const {
        return frame_size == HEADER_SIZE;
    }

    void FrameReader::reset() {
        frame_size = 0;
        read_bytes = 0;
    }

    int FrameReader::try_read(int fd, std::span<uint8_t> result_buffer) {
        if (!this->try_read_header(fd)) {
            return -1;
        }

        if (this->is_empty_payload()) {
            this->reset();
            return 0;
        }

        size_t fresh_read_bytes = read(fd, &buffer[read_bytes], frame_size - read_bytes);
        
        if (fresh_read_bytes == 0) {
            throw std::runtime_error(std::string("socket disconnected"));
        }
        
        if (fresh_read_bytes < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) {
                return false;
            } else {
                throw std::runtime_error(std::string("socket error") + std::strerror(errno));
            }
        }

        read_bytes += fresh_read_bytes;
        if (read_bytes == frame_size) {
            size_t payload_size = frame_size - HEADER_SIZE;
            if (result_buffer.size() >= payload_size) {
                std::memcpy(result_buffer.data(), &buffer[HEADER_SIZE], payload_size);
                this->reset();
                return payload_size;
            } else {
                throw std::runtime_error(std::string("buffer provided too small"));
            }
        }

        return -1;
    }
}
