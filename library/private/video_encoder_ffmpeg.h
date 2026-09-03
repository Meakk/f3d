#ifndef f3d_video_encoder_ffmpeg_h
#define f3d_video_encoder_ffmpeg_h

#include "video_encoder.h"

namespace f3d::detail
{
class video_encoder_ffmpeg : public video_encoder
{
public:
  ///@{ @name Constructors
  /**
   * Default/copy/move constructors/operators.
   * The constructor throws a codec_exception if the codec cannot be initialized.
   */
  explicit video_encoder_ffmpeg(const params& p);
  ~video_encoder_ffmpeg() override;
  video_encoder_ffmpeg(const video_encoder_ffmpeg& stream) = delete;
  video_encoder_ffmpeg& operator=(const video_encoder_ffmpeg& stream) = delete;
  video_encoder_ffmpeg(video_encoder_ffmpeg&& stream) noexcept = delete;
  video_encoder_ffmpeg& operator=(video_encoder_ffmpeg&& stream) noexcept = delete;
  ///@}

  ///@{
  /**
   * Get the width and height of the video stream.
   */
  int getWidth() const override;
  int getHeight() const override;
  ///@}

  /**
   * Asynchronously listen for encoded video packets and invoke the provided callback for each
   * packet. Thread safety is ensured, allowing concurrent calls to submit and listen.
   * Throws transport_exception if there is an error receiving packets from the encoder.
   */
  video_encoder_ffmpeg& listen(
    std::function<void(const std::shared_ptr<video_packet>&)> callback) override;

  /**
   * Send a frame to the video stream for encoding.
   * Returns true if the frame was successfully submitted, false if the encoder is busy and cannot
   * accept new frames at the moment. Thread safety is ensured, allowing concurrent calls to submit
   * and listen. Throws transport_exception if there is an error submitting the frame to the
   * encoder.
   */
  bool submit(const std::shared_ptr<video_frame>& frame) override;

  /**
   * Flush the video stream, ensuring all encoded frames are processed.
   * Signals the end of the stream to the encoder, allowing it to output any remaining packets.
   */
  video_encoder_ffmpeg& flush() override;

private:
  class internals;
  internals* Internals;
};
}

#endif
