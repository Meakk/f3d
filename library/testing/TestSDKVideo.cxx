#include "PseudoUnitTest.h"

#include <engine.h>
#include <video_encoder.h>

#include <thread>

int TestSDKVideo([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
  PseudoUnitTest test;
  f3d::log::setVerboseLevel(f3d::log::VerboseLevel::DEBUG);

  // testing all the encoders
  for (auto codec : { f3d::video_encoder::codec::H264_NVENC,
         f3d::video_encoder::codec::H264_VIDEOTOOLBOX, f3d::video_encoder::codec::H264_CPU })
  {
    for (auto compression : { f3d::video_encoder::compression::FAST,
           f3d::video_encoder::compression::BALANCED, f3d::video_encoder::compression::HIGH })
    {
      try
      {
        f3d::video_encoder encoder(
          { .Codec = codec, .Width = 300, .Height = 300, .Compression = compression });
      }
      catch (f3d::video_encoder::codec_exception&)
      {
        // if the codec is not available, it's fine
      }
    }
  }

  // testing encoder initialization failure
  // width and height are 0, should fail
  test.expect<f3d::video_encoder::codec_exception>(
    "video encoder initialization failure", [&]() { f3d::video_encoder encoder({}); });

  f3d::engine eng = f3d::engine::create(true);
  f3d::window& win = eng.getWindow();

  test.expect<f3d::video_frame::invalid_frame_exception>(
    "Uninitialized window", [&]() { f3d::video_frame uninitFrame = win.getVideoFrame(); });

  win.setSize(100, 101).render();
  test.expect<f3d::video_frame::invalid_frame_exception>(
    "Size not even", [&]() { f3d::video_frame oddDimFrame = win.getVideoFrame(); });

  test.expect<f3d::video_frame::invalid_frame_exception>(
    "Size invalid", [&]() { f3d::video_frame wrongDimFrame(0, 0); });

  test.expect<f3d::video_frame::invalid_frame_exception>(
    "Size too big", [&]() { f3d::video_frame wrongDimFrame(1500000000, 1500000000); });

  // use auto for the actual test
  f3d::video_encoder encoder({ .Codec = f3d::video_encoder::codec::H264_AUTO,
    .Width = 300,
    .Height = 300,
    .LowLatency = true });

  test("video encoder width", encoder.getWidth() == 300);
  test("video encoder height", encoder.getHeight() == 300);

  encoder.listen([](const f3d::video_packet&) {});
  encoder.listen(nullptr);

  // get a 100x100 frame but not submit it to the 300x300 encoder
  // it's there to cover the change of resolution in the frame capture class
  win.setSize(100, 100).render();
  f3d::video_frame frame = win.getVideoFrame();

  // submit the video frame to the encoder
  win.setSize(300, 300).render();

  frame = win.getVideoFrame();
  frame.setTimestamp(41);
  encoder.submit(frame);

  frame = win.getVideoFrame();
  frame.setTimestamp(42);

  while (encoder.submit(frame))
  {
    // submit the frame until the encoder queue is full
  }

  // add proper listener
  int64_t packetTS = 0;
  encoder.listen([&](const f3d::video_packet& packet) {
    packetTS = packet.getTimestamp();
    packet.isKeyFrame(); // coverage
    test("video packet size", packet.getPacketSize() > 0);
    test("video packet data", packet.getPacketData() != nullptr);
  });

  // Make sure the listener is ready before flushing
  std::this_thread::sleep_for(std::chrono::milliseconds(200));

  // flush (block until all packets are drained)
  encoder.flush();

  // submit after flush
  test.expect<f3d::video_encoder::transport_exception>(
    "Submit after flush", [&]() { encoder.submit(frame); });

  // copy/move frame coverage
  f3d::video_frame copyFrame(frame);
  f3d::video_frame moveFrame(std::move(frame));
  f3d::video_frame assignFrame(300, 300);
  assignFrame = copyFrame;
  f3d::video_frame moveAssignFrame(300, 300);
  moveAssignFrame = std::move(copyFrame);

  // copy/move packet coverage
  f3d::video_packet packet;
  f3d::video_packet copyPacket(packet);
  f3d::video_packet movePacket(std::move(packet));
  f3d::video_packet assignPacket;
  assignPacket = copyPacket;
  f3d::video_packet moveAssignPacket;
  moveAssignPacket = std::move(copyPacket);

  test("video packet timestamp", packetTS == 42);

  return test.result();
}
