/*
 * client.hpp - Manages client lifecycle.
 */

#pragma once

#include <cstddef>
#include <optional>
#include <string>

#include "client_data.hpp"
#include "framing.hpp"
#include "transport.hpp"

namespace nc {
class Client {
  public:
    Client() = default;

    /*
     * Initialize the connection to the client with the file descriptor.
     */
    void init(TcpClientHandle client);

    /*
     * Tries to recv the next payload from the stream.
     *
     * You shall check if the client is alive from time to time
     * as this function will simply return None on disconnected client.
     */
    std::optional<std::string>
    try_recv_next_payload(ClientStorage &client_storage);

    /*
     * Checks whether the client is alive.
     *
     * If it returns false, you should consider the connection dead and
     * you can recycle the client with `init`.
     */
    bool is_alive() const;

    /*
     * Resets the client to default.
     */
    void reset();

    /*
     * Returns client uid.
     */
    int uid() const;

  private:
    // Transport client handle. Might be hill-defined.
    TcpClientHandle transport_client_handle;

    // Framing.
    FrameWriter frame_writer;
    FrameReader frame_reader;

    // Raw client uid. If non-negative, we know we already authenticated client.
    int raw_uid = -1;

    // Last known state of the socket.
    bool _is_alive = false;

    bool waiting_handshake() const;
    void try_handshake(ClientStorage &client_storage);
};
} // namespace nc
