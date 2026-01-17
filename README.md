# Simple TCP Reverse Proxy
A minimal, high-performance TCP reverse proxy written in C using the `poll()` system call for non-blocking I/O multiplexing.

## Features
- I/O Multiplexing: Uses `poll()` to handle multiple concurrent connections efficiently.

- Bidirectional Forwarding: Transparently forwards traffic between the client and the remote host.

- Zero Dependencies: Uses standard Linux/POSIX headers.

## Build
Compile the source using `gcc`:
```sh
gcc -o proxy proxy.c
```

## Usage
Run the binary by specifying the local port to listen on and the destination host/port.
```sh
./proxy <local_port> <remote_host> <remote_port>
```

## Implementation Details
- Connection Pairing: Uses a fixed-size array (`connection_pair`) to map client file descriptors to their corresponding remote descriptors.

- Socket Management: Implements `SO_REUSEADDR` to allow immediate restarts of the server.

- Capacity: Currently configured to handle up to 10,240 file descriptors.
