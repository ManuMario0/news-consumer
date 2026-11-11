/*
* framing.hpp - Framing over TCP socket.
*
* TCP does not guarentee that data arrives not fragmented. It is required
* to wrap data into frames that can be clearly delimited.
*
* This implementation frame data with a length header. The maximum size of a payload
* is `MAX_PAYLOAD_SIZE`.
*/

#pragma once

#include "transport.hpp"
#include <cstddef>
#include <cstdint>
#include <span>

namespace nc {
    inline constexpr size_t HEADER_SIZE = sizeof(uint32_t);
    inline constexpr size_t MAX_PAYLOAD_SIZE = 1024;
    inline constexpr size_t MAX_FRAME_SIZE = HEADER_SIZE + MAX_PAYLOAD_SIZE;

    class FrameWriter {
        public:
            FrameWriter() = default;

            /*
            * Tries to write the payload to the file descriptor.
            *
            * If the payload cannot be fully written, it gets buffered
            * to avoid blocking the thread. You may try to send the rest
            * of the payload using `flush`.
            *
            * Returns false if the payload could not be written.
            * It can be triggere because:
            * - The payload is longer than `MAX_PAYLOAD_SIZE` ;
            * - The frame writer is not writable.
            *
            * Throws an error if the file descriptor is closed.
            */
            bool try_write(TcpClientHandle& client, const std::span<const uint8_t> payload);
            
            /*
            * Checks wether the frame writer is able to accept the next frame.
            *
            * If it is not writable, you should flush frame writer. Trying to write to
            * the frame writer when `is_writable` returns no will fail.
            */
            bool is_writable() const;

            /*
            * Tries to flush the internal buffer to the file descriptor.
            *
            * Returns true if the entire frame buffer could be flushed. In that case,
            * the frame buffer is writable. On false, the frame writer is not writable.
            *
            * Throws an error if the file descriptor is closed.
            */
            bool flush(TcpClientHandle& client);

            /*
            * Resets the frame buffer without flushing the leftover.
            */
            void reset();
        
        private:
            uint8_t buffer[MAX_FRAME_SIZE] {};
            size_t frame_size = 0;
            size_t written = 0;

            size_t bytes_left_to_write() const;
    };

    class FrameReader {
        public:
            FrameReader() = default;

            /*
            * Tries to read the next payload from the file descriptor.
            *
            * The buffer provided should be at least `MAX_PAYLOAD_SIZE` long.
            * Non-compliance may result in the function throwing an error.
            * The result will be written in the buffer, and the number of bytes written
            * is returned. If the returned value is -1, no packet was received and the buffer
            * is left unchanged.
            *
            * Throws an exception if the file descriptor is closed. It may also throw an
            * exception if the received packet is ill-formed, in which case the connection
            * should be closed to reconnect with a fresh new state.
            */
            int try_read(TcpClientHandle& client, const std::span<uint8_t> buffer);

            /*
            * Resets the frame buffer without flushing the leftover.
            */
            void reset();
        
        private:
            uint8_t buffer[MAX_FRAME_SIZE] {};
            size_t frame_size = 0;
            size_t read_bytes = 0;
            
            bool reading_header() const;
            bool try_read_header(TcpClientHandle& client);
            bool is_empty_payload() const;
    };
}
