
files := main.cpp video.cpp

all:
	clang++ $(files) -o bin -lSDL3 -lswscale -lavformat -lavcodec -lavutil 
