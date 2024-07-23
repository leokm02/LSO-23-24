FROM gcc:latest

COPY . /app

WORKDIR /app

RUN make

ENTRYPOINT ["/app/server.out"]