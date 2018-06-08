#!/bin/sh

echo
echo "Starting ester server and client and attaching gdb to mapper process:"
echo

if [ ! -e "$1" ]; then
	echo "no client program or PyNN script: $1"
	exit 1
fi

echo $(which ester)
ester --margs --debug --loglevel marocco:all config::warn &
# pass all arguments except $1 to mapping process
#./ester --margs --debug ${@:2} &
SERVER_PID=$!
sleep 2

python "$1" &
CLIENT_PID=$!
sleep 2


MPID=$(pgrep -u $USER -f "mapper --debug" | sort -n | head -n 1)
echo "Mapper PID is $MPID"

CMDFILE=tempfile
echo "
list
set attached=true
" > $CMDFILE

gdb -f -x $CMDFILE prog $MPID

rm $CMDFILE

kill $CLIENT_PID $SERVER_PID
