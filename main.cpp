extern "C" {
#include <SDL2/SDL.h>
}
#include "avlibs.hpp"
#include "video.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>

const char* imagepath = "./agnee.jpg";
const char* videopath = "./example.mkv";

int updateYUVFrameTexture(SDL_Texture* texture, AVFrame* frame) {
  return SDL_UpdateYUVTexture(texture,
			      NULL,
			      frame->data[0], frame->linesize[0],
			      frame->data[1], frame->linesize[1],
			      frame->data[2], frame->linesize[2]);
}

int main() {

  const int winy = 1080;
  const int winx = 1920;
  const int maxfps = 30;

  AVObject *obj = new AVObject(videopath);

  //int seek_value = 699;
  //if (!obj->seekTo(seek_value)) {
  //  std::cerr << "couldn't seek (main)" << std::endl;
  //}

  AVFrame* frame = obj->getCurrentFrame(); // DO NOT FREE. OBJ TAKES CARE

  // sdl stuff
  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
    std::cerr << SDL_GetError();
  }

  // the last argument is the flag, you can set it to be a vulkan or gl
  // context, this is important.
  SDL_Window* pWindow = SDL_CreateWindow("linux_vde", SDL_WINDOWPOS_CENTERED,
					 SDL_WINDOWPOS_CENTERED, winx, winy,
					 0);
  
  const Uint32 render_flags = SDL_RENDERER_ACCELERATED;

  SDL_Renderer* pMainRenderer = SDL_CreateRenderer(pWindow, -1,
						   render_flags);

  // frame to renderable texure
  // that sdl_textureaccess is sussy, streaming cuz it's not static.
  SDL_Texture* frame_texture =
    SDL_CreateTexture(pMainRenderer, SDL_PIXELFORMAT_YV12,
		      SDL_TEXTUREACCESS_STREAMING, winx, winy);

  updateYUVFrameTexture(frame_texture, frame);


  bool running = true;
  while(running) {
    SDL_Event event;

    while (SDL_PollEvent(&event)) { // goes through all events that happened
	if (event.type == SDL_QUIT) {
	  running = false;
	}
    }

    if (!obj->iterateFrame(1) ) {
      std::cerr<< "couldn't iter (main)"  << std::endl;
    }

    frame = obj->getCurrentFrame();
    updateYUVFrameTexture(frame_texture, frame);

    SDL_RenderClear(pMainRenderer);
    // the two nulls are rectangels, source and dest.
    SDL_RenderCopy(pMainRenderer, frame_texture, NULL, NULL);
    SDL_RenderPresent(pMainRenderer); // double buffer.

    // TODO: this causes problems, we cant just sleep, doesn't take into
    // account the time it took to calculate all the stuff that happened.
    // aka not consistent
    SDL_Delay(1000/maxfps);
  }

  SDL_DestroyTexture(frame_texture);
  SDL_DestroyRenderer(pMainRenderer);
  SDL_DestroyWindow(pWindow);
  SDL_Quit();
  delete obj;

  return 0;
}
