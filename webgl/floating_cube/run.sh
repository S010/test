#!/bin/sh

{
	sleep 3
	xdg-open 'http://localhost:8000'
} &

python3 -m http.server
