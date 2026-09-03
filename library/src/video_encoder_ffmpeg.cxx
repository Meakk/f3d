#include "video_encoder_ffmpeg.h"

#include "log.h"

#include "video_frame_ffmpeg.h"
#include "video_packet_ffmpeg.h"

#include <atomic>
#include <mutex>
#include <thread>

extern "C"
{
#include <libavcodec/avcodec.h>
}

namespace f3d::detail
{
class video_encoder_ffmpeg::internals
{
public:
  internals() = default;
  ~internals()
  {
    this->ForceStopThread();

    if (this->CodecContext)
    {
      avcodec_free_context(&this->CodecContext);
    }
  }

  internals(const internals&) = delete;
  internals& operator=(const internals&) = delete;

  void ForceStopThread()
  {
    if (this->ListenerWorker.joinable())
    {
      this->StopRequested.store(true);
      this->ListenerWorker.join();
    }
    this->StopRequested.store(false);
  }

  AVCodecContext* CodecContext = nullptr;
  std::thread ListenerWorker;
  std::atomic_bool StopRequested = false;
  std::mutex Mutex;
};

//----------------------------------------------------------------------------
video_encoder_ffmpeg::video_encoder_ffmpeg(const params& p)
{
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

  this->Internals = new video_encoder_ffmpeg::internals();
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
}

//----------------------------------------------------------------------------
video_encoder_ffmpeg::~video_encoder_ffmpeg()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
int video_encoder_ffmpeg::getWidth() const
{
  return this->Internals->CodecContext->width;
}

//----------------------------------------------------------------------------
int video_encoder_ffmpeg::getHeight() const
{
  return this->Internals->CodecContext->height;
}

//----------------------------------------------------------------------------
video_encoder_ffmpeg& video_encoder_ffmpeg::flush()
{
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

  return *this;
}

//----------------------------------------------------------------------------
video_encoder_ffmpeg& video_encoder_ffmpeg::listen(
  std::function<void(const std::shared_ptr<video_packet>&)> callback)
{
  this->Internals->ForceStopThread();

  if (!callback)
  {
    return *this;
  }

  this->Internals->ListenerWorker = std::thread(
    [this, callback = std::move(callback)]()
    {
      std::shared_ptr<video_packet_ffmpeg> packet = std::make_shared<video_packet_ffmpeg>();
      while (!this->Internals->StopRequested.load())
      {
        AVPacket* avPacket = static_cast<AVPacket*>(packet->GetHandle());

        // Release the previous packet if there was one.
        av_packet_unref(avPacket);

        int ret;

        {
          std::lock_guard lock(this->Internals->Mutex);
          ret = avcodec_receive_packet(this->Internals->CodecContext, avPacket);
        }

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
          // This can fail for various reasons, including memory allocation failure.
          // It's not possible to cover it in the CI, so we exclude it from coverage.
          // LCOV_EXCL_START
          char errBuf[AV_ERROR_MAX_STRING_SIZE];
          av_strerror(ret, errBuf, sizeof(errBuf));
          throw transport_exception(std::string("Error receiving packet: ") + errBuf);
          // LCOV_EXCL_STOP
        }
      }
    });

  return *this;
}

//----------------------------------------------------------------------------
bool video_encoder_ffmpeg::submit(const std::shared_ptr<video_frame>& frame)
{
  std::shared_ptr<video_frame_ffmpeg> ffmpegFrame =
    std::dynamic_pointer_cast<video_frame_ffmpeg>(frame);

  std::lock_guard lock(this->Internals->Mutex);
  const int sendResult = avcodec_send_frame(
    this->Internals->CodecContext, static_cast<const AVFrame*>(ffmpegFrame->GetHandle()));

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
  return true;
}

} // namespace f3d
