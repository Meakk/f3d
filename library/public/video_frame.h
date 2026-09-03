#ifndef f3d_video_frame_h
#define f3d_video_frame_h

#include "exception.h"
#include "export.h"

namespace f3d
{
/**
 * @class   video_frame
 * @brief   Class used to represent a video frame
 *
 * A class to represent a video frame, which is a raw video frame.
 * The video frame can be submitted to the video_encoder class.
 */
class F3D_EXPORT video_frame
{
public:
  ///@{ @name Constructors
  /**
   * Default/copy/move constructors/operators.
   * The constructor can throw a video_frame::invalid_frame_exception if the resolution
   * is not even or positive, or if F3D_MODULE_FFMPEG is not enabled.
   */
  video_frame(int width, int height);
  ~video_frame();
  video_frame(const video_frame& img);
  video_frame& operator=(const video_frame& img);
  video_frame(video_frame&& img) noexcept;
  video_frame& operator=(video_frame&& img) noexcept;
  ///@}

  /**
   * Get the Y plane pointer of the video frame.
   * Used by the window class to fill the Y plane of the video frame.
   */
  std::byte* getYPlane() const;

  /**
   * Get the UV plane pointer of the video frame.
   * Used by the window class to fill the UV plane of the video frame.
   */
  std::byte* getUVPlane() const;

  /**
   * Set the timestamp of the video frame.
   */
  video_frame& setTimestamp(int64_t timestamp);

  /**
   * Get the handle of the video frame.
   * The handle is an opaque pointer to the underlying video frame data structure.
   * It's used internally by the video_encoder class and should not be used directly.
   */
  const void* getHandle() const;

  /**
   * An exception that can be thrown
   * when video frame allocation fails.
   */
  struct invalid_frame_exception : public exception
  {
    explicit invalid_frame_exception(const std::string& what = "");
  };

private:
  class internals;
  internals* Internals;
};
}

#endif
