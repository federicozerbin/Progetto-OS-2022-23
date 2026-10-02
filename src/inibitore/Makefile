CC = gcc
CFLAGS = -Wvla -Wall -Wextra -std=c99 -pthread -lrt

MODULE = inibitore

SOURCE = $(MODULE).c
OBJECT = $(MODULE).o
EXECUTABLE = $(MODULE)_executable

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECT)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(EXECUTABLE) $(OBJECT)
