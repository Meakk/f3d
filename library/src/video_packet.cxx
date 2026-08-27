#include "video_packet.h"

#ifdef F3D_MODULE_FFMPEG
extern "C"
{
#include <libavcodec/avcodec.h>
}
#endif

namespace f3d
{
class video_packet::internals
{
public:
  internals() = default;
  ~internals()
  {
#ifdef F3D_MODULE_FFMPEG
    av_packet_unref(&this->Packet);
#endif
  }

  internals(const internals&) = delete;
  internals& operator=(const internals&) = delete;

#ifdef F3D_MODULE_FFMPEG
  AVPacket Packet{};
#endif
};

//----------------------------------------------------------------------------
video_packet::video_packet()
  : Internals(new video_packet::internals())
{
}

//----------------------------------------------------------------------------
video_packet::~video_packet()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
video_packet::video_packet(const video_packet& packet)
  : Internals(new video_packet::internals())
{
#ifdef F3D_MODULE_FFMPEG
  av_packet_ref(&this->Internals->Packet, &packet.Internals->Packet);
#endif
}

//----------------------------------------------------------------------------
video_packet& video_packet::operator=(const video_packet& packet)
{
  if (this != &packet)
  {
#ifdef F3D_MODULE_FFMPEG
    av_packet_unref(&this->Internals->Packet);
    av_packet_ref(&this->Internals->Packet, &packet.Internals->Packet);
#endif
  }
  return *this;
}

//----------------------------------------------------------------------------
video_packet::video_packet(video_packet&& packet) noexcept
  : Internals(packet.Internals)
{
  packet.Internals = new video_packet::internals();
}

//----------------------------------------------------------------------------
video_packet& video_packet::operator=(video_packet&& packet) noexcept
{
  if (this != &packet)
  {
    std::swap(this->Internals, packet.Internals);
  }
  return *this;
}

//----------------------------------------------------------------------------
const void* video_packet::getHandle() const
{
#ifdef F3D_MODULE_FFMPEG
  return &this->Internals->Packet;
#else
  return nullptr;
#endif
}

//----------------------------------------------------------------------------
size_t video_packet::getPacketSize() const
{
#ifdef F3D_MODULE_FFMPEG
  return this->Internals->Packet.size;
#else
  return 0;
#endif
}

//----------------------------------------------------------------------------
std::byte* video_packet::getPacketData() const
{
#ifdef F3D_MODULE_FFMPEG
  return reinterpret_cast<std::byte*>(this->Internals->Packet.data);
#else
  return nullptr;
#endif
}

//----------------------------------------------------------------------------
int64_t video_packet::getTimestamp() const
{
#ifdef F3D_MODULE_FFMPEG
  return this->Internals->Packet.pts;
#else
  return 0;
#endif
}

//----------------------------------------------------------------------------
bool video_packet::isKeyFrame() const
{
#ifdef F3D_MODULE_FFMPEG
  return this->Internals->Packet.flags & AV_PKT_FLAG_KEY;
#else
  return false;
#endif
}

} // namespace f3d
