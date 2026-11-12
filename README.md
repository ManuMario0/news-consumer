# news-consumer
A tiny, reliable news consumer for a custom protocol.

# Communication protocol

All packets going through the TCP connections are framed as follows:

```
| length | payload |
```

where `length` represents the length of the payload, not of the entire frame.
The length should be represented as a big-endian `uint32_t` integer.

All communications use big-endian byte order.

## Authentication

On connection, the client should send a payload that contains its UID as a `uint32_t`.
The server will reply with a sequence number (typed `uint32_t`) if the UID exists. If the UID does not exist,
the server will close the connection to the client immediately.

## Communication

The client can then start streaming news updates through the established stream.
The length of the entire payload cannot exceed `MAX_PAYLOAD_SIZE`.

# How to use

## Build

```bash
make
```

## Run

Launch the server:

```bash
./build/server
```

Launch a client:

```bash
SERVICE_ID=0 ./build/client
```

## Environment variables

The server needs to store client data to recover from a crash. By default, client data is written to `clients.dat`.
You can override the default location using the `STORAGE_PATH` environment variable.
You can also specify a custom port with the `PORT` environment variable.

The client requires a service identifier (`SERVICE_ID`) to run. It must be between `0` (inclusive) and `32` (exclusive).

# Capabilities

The server can handle up to 32 clients at a time.

The server is able to recover from a crash without losing data by storing the last sequence number in `STORAGE_PATH`.
You can test this by launching a server and a client, then killing both (as the client will get disconnected). When you restart both with the same environment variables, the server will pick up where it left off.

The client can also be killed and reconnected later; the server will pick up from the last sequence number it left off with.

# Limitations

The maximum number of clients is 32. If two clients authenticate to the server with the same service ID, the behavior
is undefined. Each client should have a unique ID.

Any anomalies in a client's data stream result in an immediate disconnection of that client; there is no recovery procedure.

If the server crashes at just the right moment, it is technically possible for a payload not to be displayed on screen even though it has been recorded as received. This is because the server records reception of the packet in the storage file before printing the payload itself to the screen.

If the host crashes and restarts, the contents of the storage file are undefined. We make use of the `MAP_SHARED` mechanism on UNIX-like systems to enable asynchronous writes to the storage file. If the host shuts down unexpectedly, the kernel cannot guarantee that dirty pages get written properly to disk.

The client is fully trusted. As such, the communication protocol does not enforce to carry the sequence number with each payload. Therefore the server does not verify that the client don't disfunction by sending two times the same packet (in the eyes of the client with the same sequence identifier). It would result in the server a sequence number different to the one of the client.
