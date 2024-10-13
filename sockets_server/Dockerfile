FROM gcc:latest

WORKDIR /server

COPY server.c .

RUN gcc -o server server.c

CMD ["./server"]
