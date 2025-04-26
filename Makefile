CC=gcc
CFLAGS=-fsanitize=address
LDFLAGS=-lGL -lGLEW -lglfw
OBJS=shaderun.o
TARGET=shaderun

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

clean:
	rm -f $(OBJS) $(TARGET)
