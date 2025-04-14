CC = gcc
CFLAGS = -Wall -Wextra -O2 -pedantic
LIBS = -lcurl -lpthread -lpcap

TARGET = icu
SRC = server.c client.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)

clean:
	rm -f $(TARGET)

# NOTE: gcc -Wall -pedantic client.c server.c -o icu -lcurl -lpcap
