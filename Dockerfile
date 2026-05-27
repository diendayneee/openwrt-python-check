FROM ubuntu:20.04

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    gcc \
    make \
    python3.9 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

COPY . .

RUN gcc -Wall -o check_python check_python.c

CMD ["./check_python"]
