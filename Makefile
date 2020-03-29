.PHONY: clean all

# OS specific linker flags for GLFW
# -Wno-unused-parameter -Wno-deprecated-declarations
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
LDFLAGS=-framework OpenGL -lglfw -framework Cocoa -framework IOKit -framework CoreVideo
CFLAGS=-Wall -Wextra -Wshadow -Wformat=2 -ansi -O2 -g -Wno-unused-parameter -Wno-comment
endif
ifeq ($(UNAME_S),Linux)
LDFLAGS=-lGL -lGLU -lm -pthread -ldl `pkg-config --libs glfw3`
CFLAGS=-Wall -Wextra -Wshadow -Wformat=2 -pedantic -ansi -O2 -g `pkg-config --cflags glfw3`
endif

all: main clean

main: main.o trackball.o glad.o

main.o: main.c trackball.h glad.h

trackball.o: trackball.c trackball.h

glad.o: glad.c glad.h khrplatform.h

clean:
	@rm -rf main.o trackball.o glad.o
