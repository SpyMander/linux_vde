#include "avlibs.hpp"
#include "video.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>

// as of now, audio is not being taken care of
AVObject::AVObject (const char* filepath) {
  // load contexts: format, codec, packet, frame.
  // get container header
  pFormatContext = avformat_alloc_context();
  if (!pFormatContext) {
    std::cerr<<"out of mem, couldn't allocate format context" <<std::endl;
  }

  //open the file, filepath is given.
  if (avformat_open_input(&pFormatContext, filepath, NULL, NULL) != 0) {
    std::cerr << "couldn't open with avformat!" << std::endl;
  }

  if(avformat_find_stream_info(pFormatContext, NULL) < 0) {
    std::cerr << "err in finding streams" << std::endl;
  }

  const AVCodec* pCodec = NULL;
  //params is fps, bit_rate, level, height, width etc
  AVCodecParameters* pCodecParams= NULL;
  videoStreamIndex = -1;
  //these guys will be filled durring the for loop.

  for (uint i=0; i < pFormatContext->nb_streams; i++) {

    AVCodecParameters* pLocalCodecParameters = NULL;
    pLocalCodecParameters = pFormatContext->streams[i]->codecpar;

    //find and get the codec that the stream is using.
    const AVCodec* pLocalCodec;
    pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);

    if (pLocalCodec == nullptr) {
      std::cerr << "codec is not some wierd shit" << '\n';
    }

    if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO
	&& videoStreamIndex == -1 ) {
      videoStreamIndex = i;
      pCodecParams = pLocalCodecParameters;
      pCodec = pLocalCodec;

      break;

    }
  } //end of search

  if (videoStreamIndex == -1) {
    std::cerr << "video stream doesn't exist in file" << std::endl;
  }

  //codec context is able to decode, pCodec is just passable struct data.
  pVCodecContext = avcodec_alloc_context3(pCodec);
  if (!pVCodecContext) {
    std::cerr << "cant make codec context, no space!" << std::endl;
  }

  // setting threads of codec
  //std::cout << "threading: " << pVCodecContext->thread_count << std::endl;
  //pVCodecContext->thread_count = 6; works but creates problems and shit.
  //pVCodecContext->thread_type = FF_THREAD_FRAME;

  //set params of codec.
  if (avcodec_parameters_to_context(pVCodecContext, pCodecParams) < 0) {
    std::cerr << "codec params couldn't be passed" << std::endl;
  }

  // open codec (?)
  if (avcodec_open2(pVCodecContext, pCodec, NULL)) {
    std::cerr << "unable to open codec" << std::endl;
  }
  pFrame = av_frame_alloc();

  if (!pFrame) {
    std::cerr << "no space for frame" << std::endl;
  }

  // pPacket is a chunk of data
  pPacket = av_packet_alloc();
  if (!pPacket) {
    std::cerr << "no space for packet" << std::endl;
  }

  // set helper stuff

  fps = av_q2d(pFormatContext->streams[videoStreamIndex]->r_frame_rate);
  fps_freq = 1/fps;
  
}

AVObject::~AVObject() {
  avformat_close_input(&pFormatContext);
  av_packet_free(&pPacket);
  av_frame_free(&pFrame);
  avcodec_free_context(&pVCodecContext);
}

void AVObject::printInfo() {
  // info that header has
  std::cout << pFormatContext->iformat->name << std::endl;
  //prints streams num audio and video.
  std::cout << "bitrate: "<< pFormatContext->bit_rate << std::endl;
  std::cout<<"stream amount: " << pFormatContext->nb_streams << std::endl;
}

AVFrame* AVObject::getCurrentFrame() {
  return pFrame; //this constantly changes.
}

bool AVObject::iterateFrame(uint amount) {
  if (amount <= 0) {
    return true; //false ???
  }

  bool ret = true;
  while (av_read_frame(pFormatContext, pPacket) >= 0) { 
    // Big note: this goes through all of the streams
    // so, 0, 1, 2, 3, 0, 1, 2, 3.

    // i hate that this is nested. i'll leave it for now.
    // for pPacket to not get fucked up, i need to do it this way.
    if (pPacket->stream_index == videoStreamIndex) {

      int send_packet_response;
      send_packet_response = avcodec_send_packet(pVCodecContext, pPacket);
      if (send_packet_response < 0) {
	std::cerr << "error while sending packet: "
		  << av_err2str(send_packet_response) << std::endl;
	ret = false;
      }


      // set the frame to decoded packet.
      // p frame was set here.
      int recieve_frame_response = avcodec_receive_frame(pVCodecContext,
							 pFrame);
      // errs
      if (recieve_frame_response == AVERROR_EOF) {
	std::cout << "the video ended!" << std::endl;
	ret = false;
      }

      // AVERROR(EAGAIN) no idea, but check for this?
      else if (recieve_frame_response < 0 ||
	       recieve_frame_response == AVERROR(EAGAIN)) {
	std::cerr << "couldn't recieve the frame: "
		  << av_err2str(recieve_frame_response) << std::endl;

	ret = false;
      }

      amount -= 1;
      if (amount <= 0){
	av_packet_unref(pPacket); // TODO: do this better
	break;
      }   
    }

    av_packet_unref(pPacket); // unref out of the if-statement
  }

  int current_frame_pos = frameNumberToPts(pVCodecContext->frame_num);
  AVRational timebase= pFormatContext->streams[videoStreamIndex]->time_base;

  //std::cout << "--------------------" << std::endl;
  //std::cout << "frame to pts" << current_frame_pos << std::endl;
  //std::cout << "actual pts: " << pFrame->pts << std::endl;
  //std::cout<<"framenum:"<<pVCodecContext->frame_num<<std::endl;
  //std::cout << "--------------------" << std::endl;
  
  return ret;
}

bool AVObject::seekTo(uint frame) {
  // too slow for some reason. the iterateFrame() after the seek is too
  // slow.
  // TODO: when locating exactly on a keyframe it misses it by 1 frame. fix
  AVRational timebase = pFormatContext->streams[videoStreamIndex]
    ->time_base;
  // frame minus 1 to compensate for iterateFrame(1);
  int64_t ts = (int64_t) ( ((frame-1)/fps) * timebase.den/timebase.num);
  int flag = AVSEEK_FLAG_BACKWARD;

  int seek_ret = av_seek_frame(pFormatContext, videoStreamIndex,
  			       ts, flag);
  if (seek_ret < 0) {
    std::cerr<< "couldn't seek: " << av_err2str(seek_ret) << std::endl;
    return false;
  }

  avcodec_flush_buffers(pVCodecContext);

  iterateFrame(1); // to get frame number

  uint after_seek_frame = getCurrentFrameNumber();
  std::cout << "after seek and iter: " << after_seek_frame << std::endl;

  uint delta = frame - after_seek_frame;
  // since after_seek_frame is behind the requested frame. 
  std::cout << "seeked (delta):" << delta << std::endl;
  
  if (frame == after_seek_frame) {
    std::cout << "seeked on I frame!" << std::endl;
    return true;
  }

  if (after_seek_frame == 0) {
    // for some reason, it will be off by 1 if not done like this.
    iterateFrame(delta-1);
    return true;
  }

  iterateFrame(delta);

  std::cout << "after all: " << getCurrentFrameNumber() << std::endl;
  return true;
}

uint AVObject::getCurrentFrameNumber() {
  if (pFrame->pts <= 0) {
    return 0;
  }

  double current_frame_seconds = pFrame->pts *
    av_q2d(pFormatContext->streams[videoStreamIndex]->time_base);
  // q2d deals with av_rationals'

  uint current_frame_pos = std::round(current_frame_seconds * fps)+1;
  // need to round and add 1 for more persicion, dunno why we need to add 1
  // though, yeah.
  return current_frame_pos;
}

long AVObject::frameNumberToPts(uint frame) {
  if (frame == 0) {
    return 0;
  }

  AVRational timebase = pFormatContext->streams[videoStreamIndex]
    ->time_base;

  long pts = (frame) / av_q2d(timebase); // frame-1
  // dunno why frame is off by one.

  pts /= fps;
  // for some reason this is off by 33
  
  
  return pts;
}

uint AVObject::ptsToFrameNumber(long pts) {
  double current_frame_seconds = pts *
    av_q2d(pFormatContext->streams[videoStreamIndex]->time_base);

  uint current_frame_pos = std::round(current_frame_seconds * fps);
  // plus 1

  return current_frame_pos;
}

double AVObject::getVideoFps() {
  return fps;
}
