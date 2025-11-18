CC := x86_64-w64-mingw32-gcc
CXX := x86_64-w64-mingw32-g++
PREFIX := /mingw64

CFLAGS := -O2 -Wall -I./src -I$(PREFIX)/include
LDFLAGS := -L$(PREFIX)/lib -lx264 -lrtmp -ld3d11 -lDXGI -ldxguid -luser32 -lgdi32

SRC_C := src/main.c src/encoder.c src/rtmp_push.c
SRC_CPP := src/capture.cpp

OBJ := $(SRC_C:.c=.o) $(SRC_CPP:.cpp=.o)

all: capture_streamer.exe

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

capture_streamer.exe: $(OBJ)
	$(CXX) -o $@ $(OBJ) $(LDFLAGS)

clean:
	rm -f $(OBJ) capture_streamer.exe
