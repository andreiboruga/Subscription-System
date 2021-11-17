
CFLAGS = -Wall -g

all: server subscriber

# Compileaza server.c
server: server.c

# Compileaza subscriber.c
subscriber: subscriber.c

.PHONY: clean

clean:
	rm -f server subscriber
