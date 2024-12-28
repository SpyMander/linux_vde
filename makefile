
files := main.cpp video.cpp

all:
	g++ $(files) -o bin -lSDL3 -lswscale -lavformat -lavcodec -lavutil 
