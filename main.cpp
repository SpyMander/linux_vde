extern "C" {
#include <SDL3/SDL.h>
}
#include "avlibs.hpp"
#include "video.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>

const char* imagepath = "./agnee.jpg";
const char* videopath = "./example.mkv";

bool updateYUVFrameTexture(SDL_Texture* texture, AVFrame* frame) {
  return SDL_UpdateYUVTexture(texture,
			      NULL,
			      frame->data[0], frame->linesize[0],
			      frame->data[1], frame->linesize[1],
			      frame->data[2], frame->linesize[2]);
}

int main() {

  const int winy = 1080;
  const int winx = 1920;

  AVObject *obj = new AVObject(videopath);
  const double maxfps = obj->getVideoFps();
  const double frame_time_msec = 1000/maxfps;

  AVFrame* frame = obj->getCurrentFrame(); // DO NOT FREE. OBJ TAKES CARE

  // sdl stuff
  if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS)) {
    std::cerr << SDL_GetError();
  }

  // the last argument is the flag, you can set it to be a vulkan or gl
  // context, this is important.
  SDL_Window* pWindow = SDL_CreateWindow("linux_vde", winx, winy, 0);
  
  SDL_Renderer* pMainRenderer = SDL_CreateRenderer(pWindow, NULL);

  // frame to renderable texurinte
  // that sdl_textureaccess is sussy, streaming cuz it's not static.
  SDL_Texture* frame_texture =
    SDL_CreateTexture(pMainRenderer, SDL_PIXELFORMAT_YV12,
		      SDL_TEXTUREACCESS_STREAMING, winx, winy);

  updateYUVFrameTexture(frame_texture, frame);


  bool running = true;
  uint calculation_msec_sum = 0;
  uint loops = 0;
  while(running) {
    uint first_tick = SDL_GetTicks();
    uint iter = 1;
    SDL_Event event;

    while (SDL_PollEvent(&event)) { // goes through all events that happened
	if (event.type == SDL_EVENT_QUIT) {
	  running = false;
	}
	if (event.key.scancode == SDL_SCANCODE_RIGHT) {
	  uint seek_amount = obj->getCurrentFrameNumber() +
		(int)obj->getVideoFps()*5;
	  obj->seekTo(seek_amount);
	  std::cout << "right: " << seek_amount << std::endl;
	  iter = 0;
	}
	if (event.key.scancode == SDL_SCANCODE_LEFT) {
          uint seek_amount = obj->getCurrentFrameNumber() -
	  (int)obj->getVideoFps()*5;
	  obj->seekTo(seek_amount);
	  std::cout << "left: " << seek_amount << std::endl;
	  iter = 0;
	}
    }

    if (!obj->iterateFrame(iter) ) {
      std::cerr<< "couldn't iter (main)"  << std::endl;
    }
    if (iter != 0) {
      frame = obj->getCurrentFrame();
      updateYUVFrameTexture(frame_texture, frame);
    }

    SDL_RenderClear(pMainRenderer);
    // the two nulls are rectangels, source and dest.
    SDL_RenderTexture(pMainRenderer, frame_texture, NULL, NULL);
    SDL_RenderPresent(pMainRenderer); // double buffer.

    uint last_tick = SDL_GetTicks();
    uint delta_tick = last_tick - first_tick;

    loops+=1;
    calculation_msec_sum += delta_tick;

    if (delta_tick < frame_time_msec) {
      SDL_Delay(frame_time_msec - delta_tick);
    }
    
    else {
      std::cout << "dropped frame (delta):" << delta_tick
	      << " frame_time_msec: " << frame_time_msec <<std::endl;
    }

  }

  double calculation_avg = calculation_msec_sum/ (double)loops;
  std::cout << calculation_avg << std::endl;

  SDL_DestroyTexture(frame_texture);
  SDL_DestroyRenderer(pMainRenderer);
  SDL_DestroyWindow(pWindow);
  SDL_Quit();
  delete obj;

  return 0;
}
