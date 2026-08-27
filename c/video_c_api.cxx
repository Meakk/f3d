#include "video_c_api.h"
#include "log.h"
#include "video_encoder.h"

//----------------------------------------------------------------------------
void f3d_video_frame_delete(f3d_video_frame_t* frame)
{
  if (!frame)
  {
    f3d::log::warn("Video frame is null, cannot delete video frame");
    return;
  }

  delete reinterpret_cast<f3d::video_frame*>(frame);
}

//----------------------------------------------------------------------------
void f3d_video_frame_set_timestamp(f3d_video_frame_t* frame, int64_t timestamp)
{
  if (!frame)
  {
    return;
  }

  f3d::video_frame* cpp_frame = reinterpret_cast<f3d::video_frame*>(frame);
  cpp_frame->setTimestamp(timestamp);
}

//----------------------------------------------------------------------------
size_t f3d_video_packet_get_packet_size(const f3d_video_packet_t* packet)
{
  if (!packet)
  {
    return 0;
  }

  const f3d::video_packet* cpp_packet = reinterpret_cast<const f3d::video_packet*>(packet);
  return cpp_packet->getPacketSize();
}

//----------------------------------------------------------------------------
const unsigned char* f3d_video_packet_get_packet_data(const f3d_video_packet_t* packet)
{
  if (!packet)
  {
    return nullptr;
  }

  const f3d::video_packet* cpp_packet = reinterpret_cast<const f3d::video_packet*>(packet);
  return reinterpret_cast<const unsigned char*>(cpp_packet->getPacketData());
}

//----------------------------------------------------------------------------
int64_t f3d_video_packet_get_timestamp(const f3d_video_packet_t* packet)
{
  if (!packet)
  {
    return 0;
  }

  const f3d::video_packet* cpp_packet = reinterpret_cast<const f3d::video_packet*>(packet);
  return cpp_packet->getTimestamp();
}

//----------------------------------------------------------------------------
F3D_EXPORT int f3d_video_packet_is_key_frame(const f3d_video_packet_t* packet)
{
  if (!packet)
  {
    return 0;
  }

  const f3d::video_packet* cpp_packet = reinterpret_cast<const f3d::video_packet*>(packet);
  return cpp_packet->isKeyFrame() ? 1 : 0;
}

//----------------------------------------------------------------------------
f3d_video_encoder_t* f3d_video_encoder_new(const f3d_video_encoder_params_t* params)
{
  if (!params)
  {
    f3d::log::warn("Video encoder params is null, cannot create video encoder");
    return nullptr;
  }

  try
  {
    f3d::video_encoder::params cppParams;
    cppParams.Codec = static_cast<f3d::video_encoder::codec>(params->codec);
    cppParams.Width = params->width;
    cppParams.Height = params->height;
    cppParams.FrameRate = params->frame_rate;
    cppParams.Compression = static_cast<f3d::video_encoder::compression>(params->compression);
    cppParams.LowLatency = params->low_latency != 0;

    f3d::video_encoder* encoder = new f3d::video_encoder(cppParams);
    return reinterpret_cast<f3d_video_encoder_t*>(encoder);
  }
  catch (const f3d::video_encoder::codec_exception& e)
  {
    f3d::log::error("Cannot create video encoder: ", e.what());
    return nullptr;
  }
}

//----------------------------------------------------------------------------
void f3d_video_encoder_delete(f3d_video_encoder_t* encoder)
{
  if (!encoder)
  {
    f3d::log::warn("Video encoder is null, cannot delete video encoder");
    return;
  }

  delete reinterpret_cast<f3d::video_encoder*>(encoder);
}

//----------------------------------------------------------------------------
int f3d_video_encoder_get_width(const f3d_video_encoder_t* encoder)
{
  if (!encoder)
  {
    return 0;
  }

  return reinterpret_cast<const f3d::video_encoder*>(encoder)->getWidth();
}

//----------------------------------------------------------------------------
int f3d_video_encoder_get_height(const f3d_video_encoder_t* encoder)
{
  if (!encoder)
  {
    return 0;
  }

  return reinterpret_cast<const f3d::video_encoder*>(encoder)->getHeight();
}

//----------------------------------------------------------------------------
void f3d_video_encoder_listen(
  f3d_video_encoder_t* encoder, f3d_video_packet_callback_t callback, void* user_data)
{
  if (!encoder || !callback)
  {
    return;
  }

  f3d::video_encoder* cpp_encoder = reinterpret_cast<f3d::video_encoder*>(encoder);
  cpp_encoder->listen([=](const f3d::video_packet& packet)
    { callback(reinterpret_cast<const f3d_video_packet_t*>(&packet), user_data); });
}

//----------------------------------------------------------------------------
int f3d_video_encoder_submit(f3d_video_encoder_t* encoder, f3d_video_frame_t* frame)
{
  if (!encoder || !frame)
  {
    return 0;
  }

  f3d::video_encoder* cpp_encoder = reinterpret_cast<f3d::video_encoder*>(encoder);
  f3d::video_frame* cpp_frame = reinterpret_cast<f3d::video_frame*>(frame);

  try
  {
    return cpp_encoder->submit(*cpp_frame) ? 1 : 0;
  }
  catch (const f3d::video_encoder::transport_exception& e)
  {
    f3d::log::error("Cannot submit video frame: ", e.what());
    return -1;
  }
}

//----------------------------------------------------------------------------
void f3d_video_encoder_flush(f3d_video_encoder_t* encoder)
{
  if (!encoder)
  {
    return;
  }

  reinterpret_cast<f3d::video_encoder*>(encoder)->flush();
}
