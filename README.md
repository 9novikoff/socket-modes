#  SOCKET_MODES

---

##  Overview

Socket Communication Performance Benchmark 

---

##  Repository Structure

---

##  Getting Started

###  Usage with docker


1. Pull server and client
```sh
 docker pull 9novikov/socket_server
```
```sh
 docker pull 9novikov/socket_client
```

2. Run containers

Run server
```sh
docker run --network host -v [your_directory]:/server/volume socket-server -mode [server_mode] -num_packets [number] -packet_size [size]
```

Run client
```sh
docker run --network host -v [your_directory]:/client/volume socket-client -mode [client_mode] -num_packets [number] -packet_size [size]
```

Note: Replace [your_directory] with the path to the directory you want to share, [server_mode] with one of the 8 server modes, [client_mode] with one of the 2 client modes, [number] with the number of packets, and [size] with the packet size.

Server Modes:

inet_sync_blocking
inet_sync_nonblocking
inet_async_blocking
inet_async_nonblocking
unix_sync_blocking
unix_sync_nonblocking
unix_async_blocking
unix_async_nonblocking

Client Modes:

inet
unix

Example:
```sh
docker run --network host -v /path/to/your_server_directory:/server/volume socket-server -mode inet_sync_blocking -num_packets 10000 -packet_size 100
docker run --network host -v /path/to/your_client_directory:/client/volume socket-client -mode inet -num_packets 10000 -packet_size 100
```
