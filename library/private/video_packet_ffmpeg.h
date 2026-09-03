#ifndef f3d_video_packet_ffmpeg_h
#define f3d_video_packet_ffmpeg_h

#include "video_packet.h"

namespace f3d::detail
{
/**
 * @class   video_packet_ffmpeg
 * @brief   Class used to represent a video packet
 *
 * A class to represent a video packet, which is an encoded video frame.
 * The video packet can be used to write to a file or stream over a network.
 * The video packet is produced by the video_encoder class.
 */
class F3D_EXPORT video_packet_ffmpeg : public video_packet
{
public:
  ///@{ @name Constructors
  /**
   * Default/copy/move constructors/operators.
   */
  video_packet_ffmpeg();
  ~video_packet_ffmpeg() override;
  video_packet_ffmpeg(const video_packet_ffmpeg& packet) = delete;
  video_packet_ffmpeg& operator=(const video_packet_ffmpeg& packet) = delete;
  video_packet_ffmpeg(video_packet_ffmpeg&& packet) noexcept = delete;
  video_packet_ffmpeg& operator=(video_packet_ffmpeg&& packet) noexcept = delete;
  ///@}

  /**
   * Get the size of the video packet in bytes.
   */
  size_t getPacketSize() const override;

  /**
   * Get the data of the video packet.
   */
  std::byte* getPacketData() const override;

  /**
   * Get the timestamp of the video packet.
   */
  int64_t getTimestamp() const override;

  /**
   * Returns true if the packet is a key frame, false otherwise.
   */
  bool isKeyFrame() const override;

  /**
   * Implementation only API.
   * Get the handle of the video packet.
   * The handle is an opaque pointer to the underlying video packet data structure.
   * It's used internally by the video_encoder class and should not be used directly.
   */
  void* GetHandle();

private:
  class internals;
  internals* Internals;
};
}

#endif
