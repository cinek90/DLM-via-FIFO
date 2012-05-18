#!/bin/sh
if ! [ -p /tmp/DLM/DLMfifo ]
	then
	../bin/DLM > DLM.log &
fi
for skrypt in $*
do
	./$skrypt
done
