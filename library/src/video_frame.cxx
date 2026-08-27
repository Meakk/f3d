#include "video_frame.h"

#ifdef F3D_MODULE_FFMPEG
extern "C"
{
#include <libavcodec/avcodec.h>
}
#endif

namespace f3d
{
class F3D_EXPORT video_frame::internals
{
public:
  internals() = default;
  ~internals()
  {
#ifdef F3D_MODULE_FFMPEG
    av_frame_unref(&this->Frame);
#endif
  }

  internals(const internals&) = delete;
  internals& operator=(const internals&) = delete;

#ifdef F3D_MODULE_FFMPEG
  AVFrame Frame{};
#endif
};

//----------------------------------------------------------------------------
video_frame::video_frame(int width, int height)
{
  if (width <= 0 || height <= 0)
  {
    throw invalid_frame_exception("Video frame size must be positive");
  }

  if (width % 2 != 0 || height % 2 != 0)
  {
    throw invalid_frame_exception("Video frame size must be even");
  }

#ifdef F3D_MODULE_FFMPEG
  this->Internals = new video_frame::internals();
  this->Internals->Frame.format = AV_PIX_FMT_NV12;
  this->Internals->Frame.width = width;
  this->Internals->Frame.height = height;
  this->Internals->Frame.linesize[0] = width;
  this->Internals->Frame.linesize[1] = width;

  if (av_frame_get_buffer(&this->Internals->Frame, 32) < 0)
  {
    delete this->Internals;
    throw invalid_frame_exception("Failed to allocate video frame buffer");
  }
#else
  throw invalid_frame_exception("Video frame requires F3D_MODULE_FFMPEG to be enabled");
#endif
}

//----------------------------------------------------------------------------
video_frame::~video_frame()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
video_frame::video_frame(const video_frame& img)
  : Internals(new video_frame::internals())
{
#ifdef F3D_MODULE_FFMPEG
  av_frame_ref(&this->Internals->Frame, &img.Internals->Frame);
#endif
}

//----------------------------------------------------------------------------
video_frame& video_frame::operator=(const video_frame& img)
{
  if (this != &img)
  {
#ifdef F3D_MODULE_FFMPEG
    av_frame_unref(&this->Internals->Frame);
    av_frame_ref(&this->Internals->Frame, &img.Internals->Frame);
#endif
  }
  return *this;
}

//----------------------------------------------------------------------------
video_frame::video_frame(video_frame&& img) noexcept
  : Internals(img.Internals)
{
  img.Internals = new video_frame::internals();
}

//----------------------------------------------------------------------------
video_frame& video_frame::operator=(video_frame&& img) noexcept
{
  if (this != &img)
  {
    std::swap(this->Internals, img.Internals);
  }
  return *this;
}

//----------------------------------------------------------------------------
std::byte* video_frame::getYPlane() const
{
#ifdef F3D_MODULE_FFMPEG
  return reinterpret_cast<std::byte*>(this->Internals->Frame.data[0]);
#else
  return nullptr;
#endif
}

//----------------------------------------------------------------------------
std::byte* video_frame::getUVPlane() const
{
#ifdef F3D_MODULE_FFMPEG
  return reinterpret_cast<std::byte*>(this->Internals->Frame.data[1]);
#else
  return nullptr;
#endif
}

//----------------------------------------------------------------------------
video_frame& video_frame::setTimestamp(int64_t timestamp)
{
#ifdef F3D_MODULE_FFMPEG
  this->Internals->Frame.pts = timestamp;
#endif
  return *this;
}

//----------------------------------------------------------------------------
const void* video_frame::getHandle() const
{
#ifdef F3D_MODULE_FFMPEG
  return &this->Internals->Frame;
#else
  return nullptr;
#endif
}

//----------------------------------------------------------------------------
video_frame::invalid_frame_exception::invalid_frame_exception(const std::string& what)
  : exception(what)
{
}
} // namespace f3d
