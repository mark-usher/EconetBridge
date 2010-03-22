# $Revision:$
# Use recursive make to build for unix and avr
# No dependencies between builds so recursive is OK here

# SLIP settings
LOCAL_IP =  192.168.3.1
REMOTE_IP = 192.168.3.2
BAUDRATE =  38400
SLIP_DEV = /dev/ttyS0

all: unix avr

# Create a build directory if necessary and do a make in that
unix:
	mkdir -p obj-unix
	make -C obj-unix -f ../Makefile.unix

# Create a build directory if necessary and do a make in that
avr:
	mkdir -p obj-avr
	make -C obj-avr -f ../Makefile.avr

# Set up slip connections
slip:
	setserial ${SLIP_DEV} low_latency
	slattach -s ${BAUDRATE} -p slip ${SLIP_DEV} &
	sleep 1 # allow time for slip device to appear
	ifconfig sl0 ${LOCAL_IP} pointopoint ${REMOTE_IP} up

# clean just removes the build directories
clean:
	rm -rf obj-unix
	rm -rf obj-avr

.PHONY: unix avr clean sl0 sl1

