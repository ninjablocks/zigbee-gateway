DEVICE = COORDINATOR
#DEVICE = ROUTER
#DEVICE = ENDDEV

#Relative project path
PROJ_DIR = 

INCLUDE = -I$(PROJ_DIR)../../../../server/Source -I$(PROJ_DIR)../Source -I$(PROJ_DIR)../../Source
LIBS = -lpthread

CC=gcc

CFLAGS= -c -Wall -g -std=gnu99

all: rmdev.bin

rmdev.bin: rmdev.o socket_client.o SimpleDB.o
	$(CC) rmdev.o socket_client.o SimpleDB.o $(LIBS) -o rmdev.bin

# rule for file "rmdev.o".
rmdev.o: ../Source/rmdev.c
	$(CC) $(CFLAGS) $(INCLUDE) $(DEFS) $(PROJ_DIR)../Source/rmdev.c

# rule for file "SimpleDB.o".
SimpleDB.o: ../../../../server/Source/SimpleDB.c
	$(CC) $(CFLAGS) $(INCLUDE) $(DEFS) $(PROJ_DIR)../../../../server/Source/SimpleDB.c

# rule for file "socket_client.o".
socket_client.o: $(PROJ_DIR)../../Source/socket_client.h $(PROJ_DIR)../../Source/socket_client.c
	$(CC) $(CFLAGS) $(INCLUDE) $(DEFS) $(PROJ_DIR)../../Source/socket_client.c

# rule for cleaning files generated during compilations.
clean:
	/bin/rm -f rmdev.bin *.o
