
SBU_REV= "0.1"

#GCC = gcc
GCC=gcc

CFLAGS = -Wall -DVERSION_NUMBER=${SBU_REV}
OBJECTS = zbSocController.o zbSocCmd.o interface_devicelist.o interface_grouplist.o interface_scenelist.o interface_srpcserver.o socket_server.o
LIBS = -lrt -lcurses

DEFS += -D_GNU_SOURCE

APP_NAME=zbGateway.bin

.PHONY: all, clean

${APP_NAME}: ${OBJECTS}
	$(GCC)  $(LIBS) $(CFLAGS) $(OBJECTS) -o ${APP_NAME}

%.o: ../Source/%.c
	${GCC}  $(LIBS) ${CFLAGS} ${INCLUDE} ${DEFS} -c -o $@ $<

all: ${APP_NAME}

clean:
	rm -rf *.o ${APP_NAME}
