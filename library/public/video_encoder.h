#ifndef f3d_video_encoder_h
#define f3d_video_encoder_h

#include "exception.h"
#include "export.h"
#include "video_frame.h"
#include "video_packet.h"

/// @cond
#include <functional>
/// @endcond

namespace f3d
{
/**
 * @class   video_encoder
 * @brief   Class used to represent a video stream
 *
 * A class to represent a video stream, used to encode frames
 * to video packets.
 */
class F3D_EXPORT video_encoder
{
public:
  /**
   * Enumeration of codec types.
   * It's recommended to use H264_AUTO for automatic selection of the best available codec.
   * H264_NVENC is for NVIDIA GPU hardware acceleration, H264_VIDEOTOOLBOX is for Apple hardware
   * acceleration, and H264_CPU is for CPU-based encoding using libx264.
   */
  enum class codec
  {
    H264_AUTO,
    H264_NVENC,
    H264_VIDEOTOOLBOX,
    H264_CPU,
  };

  /**
   * Enumeration of compression modes.
   */
  enum class compression
  {
    FAST,
    BALANCED,
    HIGH,
  };

  /**
   * Parameters for configuring the video stream construction.
   */
  struct params
  {
    codec Codec = codec::H264_AUTO;
    int Width = 0;
    int Height = 0;
    double FrameRate = 30;
    compression Compression = compression::FAST;
    bool LowLatency = false;
  };

  ///@{ @name Constructors
  /**
   * Default/copy/move constructors/operators.
   * The constructor throws a codec_exception if the codec cannot be initialized.
   */
  explicit video_encoder(const params& p);
  ~video_encoder();
  video_encoder(const video_encoder& stream) = delete;
  video_encoder& operator=(const video_encoder& stream) = delete;
  ///@}

  ///@{
  /**
   * Get the width and height of the video stream.
   */
  int getWidth() const;
  int getHeight() const;
  ///@}

  /**
   * Asynchronously listen for encoded video packets and invoke the provided callback for each
   * packet. Thread safety is ensured, allowing concurrent calls to submit and listen.
   * Throws transport_exception if there is an error receiving packets from the encoder.
   */
  video_encoder& listen(std::function<void(const video_packet&)> callback);

  /**
   * Send a frame to the video stream for encoding.
   * Returns true if the frame was successfully submitted, false if the encoder is busy and cannot
   * accept new frames at the moment. Thread safety is ensured, allowing concurrent calls to submit
   * and listen. Throws transport_exception if there is an error submitting the frame to the
   * encoder.
   */
  bool submit(const video_frame& frame);

  /**
   * Flush the video stream, ensuring all encoded frames are processed.
   * Signals the end of the stream to the encoder, allowing it to output any remaining packets.
   */
  video_encoder& flush();

  /**
   * An exception that can be thrown by the engine
   * when codec initialization fails.
   */
  struct codec_exception : public exception
  {
    explicit codec_exception(const std::string& what = "");
  };

  /**
   * An exception that can be thrown by the engine
   * when data transport fails.
   */
  struct transport_exception : public exception
  {
    explicit transport_exception(const std::string& what = "");
  };

private:
  class internals;
  internals* Internals;

  int receive(const video_packet& packet) const;
};
}

#endif
