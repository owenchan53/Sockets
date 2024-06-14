# Compiler to use
CC=g++

# Compiler flags, e.g., -g for debug, -O2 for optimise, -Wall for all warnings
CFLAGS=-g -O2 -Wall

# The build target executables
TARGET_SERVER=mtserver
TARGET_CLIENT=client

all: $(TARGET_SERVER) $(TARGET_CLIENT)

$(TARGET_SERVER): mtserver.cc
	$(CC) $(CFLAGS) -o $(TARGET_SERVER) mtserver.cc

$(TARGET_CLIENT): client.cc
	$(CC) $(CFLAGS) -o $(TARGET_CLIENT) client.cc

clean:
	$(RM) $(TARGET_SERVER) $(TARGET_CLIENT)
