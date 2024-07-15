
#! /bin/bash

#Script that launches n instances of client.
for ((i=0; i<$1; i++)); do
	./client &
done

wait
echo "Script executed."
