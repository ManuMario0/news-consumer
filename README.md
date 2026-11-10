# news-consumer
A tiny reliable news consumer for a custom protocol.

# Communication protocol

All packets that are going through the TCP connections are framed as follow:

```
| length | payload |
```

where length represent the length of the payload, not of the entire frame.
The length should be represented as a big-endian uint32_t integer.

All communications will be represented using big-endian byte order.

## Authentication

On connection, the client should send a payload that contains its UID as `int`.
The server will reply with a sequence number - typed `size_t` - if the UID exists. If the UID does not exists,
the server will close the connection to the client immediately.

## Communication

The client then can start streaming the news updates through the established stream.
The data should have the following format:

```
| sequence_token | data |
```

The length of the entire payload cannot exceed `MAX_PAYLOAD_SIZE`.