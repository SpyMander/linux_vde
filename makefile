
files := main.cpp video.cpp

all:
	g++ $(files) -o bin -lSDL2 -lswscale -lavformat -lavcodec -lavutil 
