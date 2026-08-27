#include "video_encoder.h"

#include "log.h"

#include <atomic>
#include <mutex>
#include <thread>

#ifdef F3D_MODULE_FFMPEG
extern "C"
{
#include <libavcodec/avcodec.h>
}
#endif

namespace f3d
{
class video_encoder::internals
{
public:
  internals() = default;
  ~internals()
  {
    this->ForceStopThread();

#ifdef F3D_MODULE_FFMPEG
    if (this->CodecContext)
    {
      avcodec_free_context(&this->CodecContext);
    }
#endif
  }

  internals(const internals&) = delete;
  internals& operator=(const internals&) = delete;

  void ForceStopThread()
  {
#ifdef F3D_MODULE_FFMPEG
    if (this->ListenerWorker.joinable())
    {
      this->StopRequested.store(true);
      this->ListenerWorker.join();
    }
    this->StopRequested.store(false);
#endif
  }

#ifdef F3D_MODULE_FFMPEG
  AVCodecContext* CodecContext = nullptr;
  std::thread ListenerWorker;
  std::atomic_bool StopRequested = false;
  std::mutex Mutex;
#endif
};

//----------------------------------------------------------------------------
video_encoder::video_encoder(const params& p)
{
#ifdef F3D_MODULE_FFMPEG
  const AVCodec* currentCodec = nullptr;
  switch (p.Codec)
  {
    case codec::H264_AUTO:
    {
      currentCodec = avcodec_find_encoder_by_name("h264_nvenc");
      if (!currentCodec)
      {
        currentCodec = avcodec_find_encoder_by_name("h264_videotoolbox");
      }
      if (!currentCodec)
      {
        currentCodec = avcodec_find_encoder_by_name("libx264");
      }
    }
    break;
    case codec::H264_NVENC:
      currentCodec = avcodec_find_encoder_by_name("h264_nvenc");
      break;
    case codec::H264_VIDEOTOOLBOX:
      currentCodec = avcodec_find_encoder_by_name("h264_videotoolbox");
      break;
    case codec::H264_CPU:
    default:
      currentCodec = avcodec_find_encoder_by_name("libx264");
      break;
  }

  if (currentCodec)
  {
    f3d::log::info("Using video encoder: ", currentCodec->name);
  }
  else
  {
    throw codec_exception("No valid encoder found");
  }

  this->Internals = new video_encoder::internals();
  this->Internals->CodecContext = avcodec_alloc_context3(currentCodec);
  if (!this->Internals->CodecContext)
  {
    // Can only fail on memory allocation failure.
    // LCOV_EXCL_START
    delete this->Internals;
    throw codec_exception("Failed to allocate the encoder context");
    // LCOV_EXCL_STOP
  }

  std::string codecName = currentCodec->name;

  this->Internals->CodecContext->width = p.Width;
  this->Internals->CodecContext->height = p.Height;
  this->Internals->CodecContext->pix_fmt = AV_PIX_FMT_NV12;
  this->Internals->CodecContext->color_range = AVCOL_RANGE_MPEG;
  this->Internals->CodecContext->time_base = av_d2q(1.0 / p.FrameRate, 1000000);
  this->Internals->CodecContext->framerate = av_d2q(p.FrameRate, 1000000);

  AVDictionary* opts = nullptr;

  if (p.LowLatency)
  {
    this->Internals->CodecContext->max_b_frames = 0;

    if (codecName == "libx264")
    {
      av_dict_set(&opts, "tune", "zerolatency", 0);
    }
    // Encoders below are not available in the CI.
    // LCOV_EXCL_START
    else if (codecName == "h264_nvenc")
    {
      av_dict_set(&opts, "tune", "ll", 0);
    }
    else if (codecName == "h264_videotoolbox")
    {
      av_dict_set(&opts, "realtime", "1", 0);
    }
    // LCOV_EXCL_STOP
  }

  switch (p.Compression)
  {
    case compression::FAST:
      if (codecName == "libx264")
      {
        av_dict_set(&opts, "preset", "veryfast", 0);
      }
      // Encoders below are not available in the CI.
      // LCOV_EXCL_START
      else if (codecName == "h264_nvenc")
      {
        av_dict_set(&opts, "preset", "p1", 0);
      }
      // LCOV_EXCL_STOP
      break;
    case compression::BALANCED:
      if (codecName == "libx264")
      {
        av_dict_set(&opts, "preset", "medium", 0);
      }
      // Encoders below are not available in the CI.
      // LCOV_EXCL_START
      else if (codecName == "h264_nvenc")
      {
        av_dict_set(&opts, "preset", "p4", 0);
      }
      // LCOV_EXCL_STOP
      break;
    case compression::HIGH:
    default:
      if (codecName == "libx264")
      {
        av_dict_set(&opts, "preset", "slow", 0);
      }
      // Encoders below are not available in the CI.
      // LCOV_EXCL_START
      else if (codecName == "h264_nvenc")
      {
        av_dict_set(&opts, "preset", "p7", 0);
      }
      // LCOV_EXCL_STOP
      break;
  }

  if (avcodec_open2(this->Internals->CodecContext, currentCodec, &opts) < 0)
  {
    avcodec_free_context(&this->Internals->CodecContext);
    av_dict_free(&opts);
    delete this->Internals;
    throw codec_exception("Failed to initialize the video encoder");
  }
  av_dict_free(&opts);
#else
  throw codec_exception("Video encoder requires F3D_MODULE_FFMPEG to be enabled");
#endif
}

//----------------------------------------------------------------------------
video_encoder::~video_encoder()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
int video_encoder::getWidth() const
{
#ifdef F3D_MODULE_FFMPEG
  return this->Internals->CodecContext->width;
#else
  return -1;
#endif
}

//----------------------------------------------------------------------------
int video_encoder::getHeight() const
{
#ifdef F3D_MODULE_FFMPEG
  return this->Internals->CodecContext->height;
#else
  return -1;
#endif
}

//----------------------------------------------------------------------------
video_encoder& video_encoder::flush()
{
#ifdef F3D_MODULE_FFMPEG
  if (this->Internals->CodecContext)
  {
    std::lock_guard lock(this->Internals->Mutex);
    avcodec_send_frame(this->Internals->CodecContext, nullptr);
  }

  // Wait for the listener thread to finish processing all packets.
  if (this->Internals->ListenerWorker.joinable())
  {
    this->Internals->ListenerWorker.join();
  }
#endif

  return *this;
}

//----------------------------------------------------------------------------
int video_encoder::receive(const video_packet& packet) const
{
#ifdef F3D_MODULE_FFMPEG
  AVPacket* avPacket = const_cast<AVPacket*>(static_cast<const AVPacket*>(packet.getHandle()));

  // Release the previous packet if there was one.
  av_packet_unref(avPacket);

  std::lock_guard lock(this->Internals->Mutex);
  return avcodec_receive_packet(this->Internals->CodecContext, avPacket);
#else
  return -1;
#endif
}

//----------------------------------------------------------------------------
video_encoder& video_encoder::listen(std::function<void(const video_packet&)> callback)
{
#ifdef F3D_MODULE_FFMPEG
  this->Internals->ForceStopThread();

  if (!callback)
  {
    return *this;
  }

  this->Internals->ListenerWorker = std::thread(
    [this, callback = std::move(callback)]()
    {
      video_packet packet;
      while (!this->Internals->StopRequested.load())
      {
        int ret = this->receive(packet);

        if (ret == AVERROR(EAGAIN))
        {
          // no frame available
          continue;
        }
        else if (ret == AVERROR_EOF)
        {
          // flush completed, no more frames
          return;
        }
        else if (ret == 0)
        {
          callback(packet);
        }
        else
        {
          char errBuf[AV_ERROR_MAX_STRING_SIZE];
          av_strerror(ret, errBuf, sizeof(errBuf));
          throw transport_exception(std::string("Error receiving packet: ") + errBuf);
        }
      }
    });
#endif

  return *this;
}

//----------------------------------------------------------------------------
bool video_encoder::submit(const video_frame& frame)
{
#ifdef F3D_MODULE_FFMPEG
  std::lock_guard lock(this->Internals->Mutex);
  const int sendResult = avcodec_send_frame(
    this->Internals->CodecContext, static_cast<const AVFrame*>(frame.getHandle()));

  if (sendResult == AVERROR(EAGAIN))
  {
    // The encoder still has queued packets, drain those before submitting more frames.
    return false;
  }

  if (sendResult < 0)
  {
    char errBuf[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(sendResult, errBuf, sizeof(errBuf));
    throw transport_exception(std::string("Failed to submit video frame to codec: ") + errBuf);
  }
#endif
  return true;
}

//----------------------------------------------------------------------------
video_encoder::codec_exception::codec_exception(const std::string& what)
  : exception(what)
{
}

//----------------------------------------------------------------------------
video_encoder::transport_exception::transport_exception(const std::string& what)
  : exception(what)
{
}

} // namespace f3d
