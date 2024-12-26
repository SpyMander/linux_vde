extern "C" {
#include <raylib.h>
}
#include "avlibs.hpp"
#include "video.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>

const char* imagepath = "./agnee.jpg";
const char* videopath = "./example.mkv";



Texture* frameToTexture(AVFrame* frame) {

    Image img = {
      .data = frame->data[0],
      .width = frame->width,
      .height = frame->height,
      .mipmaps = 1,
      .format = PIXELFORMAT_UNCOMPRESSED_GRAYSCALE,
      //PIXELFORMAT_UNCOMPRESSED_R8G8B8A8
      //PIXELFORMAT_UNCOMPRESSED_GRAYSCALE
    };
    
    //ImageResize(pImg, winx, winy);

    int texturesize = sizeof(Texture) + frame->linesize[0] * frame->height;

    //std::cout<<"tex alloc amount: " << texturesize << '\n';

    // since the image is in the stack,
    // raylib has a difficult time dealing with it.
    // so there is a sigsev error. double free (stack and manual)

    Texture* texture = (Texture*) malloc(texturesize);
    texture[0] = LoadTextureFromImage(img);

    // maybe instead of allocing a new texture everytime,
    // it's possible to change the image->data?
    return texture;
}

int main() {

  const int winy = 1080;
  const int winx = 1920;

  //AVFrame* frame = getVideoFrameRaw(videopath);
  //AVFrame* frame = yuv420torgb8(YUVFrame);
  //av_frame_free(&YUVFrame);
  //std::cout << "frame format: " << frame->format << '\n';

  SetTraceLogLevel(LOG_ERROR);
  InitWindow(winx, winy, "Best Video Player");

  SetTargetFPS(30);

  int frame_offset = 0;

  //for (int i = 0; i<770; i++) {
  // AVObject *obj = new AVObject(videopath);
  // if (i%10) {
  //  std::cout << "rest--------------------------" << std::endl;
  //  sleep(2);
  //}
  //delete obj;
  //}

  AVObject *obj = new AVObject(videopath);

  int seek_value = 0;
  
  while (!WindowShouldClose()) {
    if (!obj->seekTo(seek_value)) {
      std::cerr << "couldn't seek (main)" << std::endl;
    }
    seek_value += 1;
    
    AVFrame* frame = obj->getCurrentFrame(); // DO NOT FREE. OBJ TAKES CARE
    // we are only using y plane for now

    Texture* texture = frameToTexture(frame);

    BeginDrawing();
    ClearBackground(BLUE);

    DrawTexture(*texture, 0, 0, WHITE);
    EndDrawing();

    frame_offset += 1;

    UnloadTexture(*texture);
    free(texture);
  }

  delete obj;

  return 0;
}
