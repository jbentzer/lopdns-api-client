# Makefile for the network-timestamps applications

# Compiler and flags
CC = g++
CFLAGS = -Wall -O2 -std=c++17 
INC = -I/usr/local/include -I./3rd-party/plog/include
LIB = -L/usr/local/lib
LIBS += -lrestclient-cpp


# Source and output
SRC = lopdns-api-client.cpp lopdnsclient.cpp
OUT = lopdns-api-client

# Default target (executed when you just run `make`)
all: $(OUT)

# build lopdns-api-client
$(OUT): $(SRC)
	$(CC) -o $(OUT) $(CFLAGS) $(INC) $(SRC) $(LIB) $(LIBS)
	@echo "Build of lopdns-api-client complete!"

# Second target: clean up generated files
clean:
	rm -f $(OUT)
	@echo "Cleaned up."
