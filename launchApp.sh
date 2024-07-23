#! /bin/bash

docker compose up --build
docker cp MiaoMarket:/app/logs/log.txt log.txt
