#! /bin/bash

#Script that launches n instances of client.
for ((i=0; i<$1; i++)); do
	./client.out &
done

wait
echo "Script executed."
