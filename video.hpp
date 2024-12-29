#include "avlibs.hpp"

class AVObject {
public:
  AVObject(const char* filepath);
  ~AVObject();
  void printInfo();
  AVFrame* getCurrentFrame();
  bool iterateFrame(uint amount);
  bool seekTo(uint frame);
  uint getCurrentFrameNumber();
  long frameNumberToPts(uint frame);
  uint ptsToFrameNumber(long pts);
  double getVideoFps();
  //bool setFrame(int64_t frame_number);
private:
  AVFormatContext* pFormatContext;
  AVCodecContext* pVCodecContext;
  AVFrame* pFrame;
  AVPacket* pPacket;
  int videoStreamIndex;
  double fps;
  double fps_freq;
};
