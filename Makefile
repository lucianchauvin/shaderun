CC=gcc
CFLAGS=-fsanitize=address -fno-strict-aliasing -fno-stack-protector -fpie
LDFLAGS=-lGL -lGLEW -lglfw
OBJS=shaderun.o

shaderun: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
