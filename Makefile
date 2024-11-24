all: clean make

make: shaderun.c
	gcc shaderun.c -o shaderun -lGL -lGLEW -lglfw

clean:
	rm -f shaderun
