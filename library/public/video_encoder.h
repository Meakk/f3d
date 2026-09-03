#ifndef f3d_video_encoder_h
#define f3d_video_encoder_h

#include "exception.h"
#include "export.h"
#include "video_frame.h"
#include "video_packet.h"

/// @cond
#include <functional>
#include <memory>
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
  virtual ~video_encoder() = default;

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

  /**
   * Create a video encoder instance.
   * Throws codec_exception if the codec cannot be initialized.
   */
  [[nodiscard]] static std::shared_ptr<video_encoder> create(const params& p);

  ///@{
  /**
   * Get the width and height of the video stream.
   */
  [[nodiscard]] virtual int getWidth() const = 0;
  [[nodiscard]] virtual int getHeight() const = 0;
  ///@}

  /**
   * Asynchronously listen for encoded video packets and invoke the provided callback for each
   * packet. Thread safety is ensured, allowing concurrent calls to submit and listen.
   * Throws transport_exception if there is an error receiving packets from the encoder.
   */
  virtual video_encoder& listen(
    std::function<void(const std::shared_ptr<video_packet>&)> callback) = 0;

  /**
   * Send a frame to the video stream for encoding.
   * Returns true if the frame was successfully submitted, false if the encoder is busy and cannot
   * accept new frames at the moment. Thread safety is ensured, allowing concurrent calls to submit
   * and listen. Throws transport_exception if there is an error submitting the frame to the
   * encoder.
   */
  virtual bool submit(const std::shared_ptr<video_frame>& frame) = 0;

  /**
   * Flush the video stream, ensuring all encoded frames are processed.
   * Signals the end of the stream to the encoder, allowing it to output any remaining packets.
   */
  virtual video_encoder& flush() = 0;

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
};
}

#endif
