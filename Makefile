CC = gcc
CFLAGS = -Wall -Wextra -O2
LIBS = -lcurl -lpthread

TARGET = server
SRC = server.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)

clean:
	rm -f $(TARGET)

