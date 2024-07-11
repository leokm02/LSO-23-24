#! /bin/bash

#Script that launches 3 instances of client, 1 per second.

./client & 
sleep 1 
./client & 
sleep 1 
./client & 
sleep 1

wait
echo "Script executed."
