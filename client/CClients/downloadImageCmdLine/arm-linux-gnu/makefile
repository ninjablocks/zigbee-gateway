DEVICE = COORDINATOR
#DEVICE = ROUTER
#DEVICE = ENDDEV

#Relative project path
PROJ_DIR = 

INCLUDE = -I$(PROJ_DIR)../../../../server/Source -I$(PROJ_DIR)../Source -I$(PROJ_DIR)../../Source
LIBS = -lpthread

CC= /opt/toolchains/arm-linaro-12/bin/arm-none-linux-gnueabi-gcc

CFLAGS= -c -Wall -g -std=gnu99

all: dlimage.bin

dlimage.bin: dlimage.o socket_client.o
	$(CC) dlimage.o socket_client.o $(LIBS) -o dlimage.bin

# rule for file "dlimage.o".
dlimage.o: ../Source/dlimage.c
	$(CC) $(CFLAGS) $(INCLUDE) $(DEFS) $(PROJ_DIR)../Source/dlimage.c

# rule for file "socket_client.o".
socket_client.o: $(PROJ_DIR)../../Source/socket_client.h $(PROJ_DIR)../../Source/socket_client.c
	$(CC) $(CFLAGS) $(INCLUDE) $(DEFS) $(PROJ_DIR)../../Source/socket_client.c

# rule for cleaning files generated during compilations.
clean:
	/bin/rm -f dlimage.bin *.o
