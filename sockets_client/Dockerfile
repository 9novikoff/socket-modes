FROM gcc:latest

WORKDIR /client

COPY client.c .

RUN gcc -o client client.c

CMD ["./client"]
