#ifndef f3d_video_packet_h
#define f3d_video_packet_h

#include "exception.h"
#include "export.h"

namespace f3d
{
/**
 * @class   video_packet
 * @brief   Class used to represent a video packet
 *
 * A class to represent a video packet, which is an encoded video frame.
 * The video packet can be used to write to a file or stream over a network.
 * The video packet is produced by the video_encoder class.
 */
class F3D_EXPORT video_packet
{
public:
  ///@{ @name Constructors
  /**
   * Default/copy/move constructors/operators.
   */
  video_packet();
  ~video_packet();
  video_packet(const video_packet& packet);
  video_packet& operator=(const video_packet& packet);
  video_packet(video_packet&& packet) noexcept;
  video_packet& operator=(video_packet&& packet) noexcept;
  ///@}

  /**
   * Get the size of the video packet in bytes.
   */
  size_t getPacketSize() const;

  /**
   * Get the data of the video packet.
   */
  std::byte* getPacketData() const;

  /**
   * Get the timestamp of the video packet.
   */
  int64_t getTimestamp() const;

  /**
   * Returns true if the packet is a key frame, false otherwise.
   */
  bool isKeyFrame() const;

  /**
   * Get the handle of the video packet.
   * The handle is an opaque pointer to the underlying video packet data structure.
   * It's used internally by the video_encoder class and should not be used directly.
   */
  const void* getHandle() const;

private:
  class internals;
  internals* Internals;
};
}

#endif
