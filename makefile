
files := main.cpp video.cpp

all:
	g++ $(files) -o bin -lraylib -lswscale -lavformat -lavcodec -lavutil 