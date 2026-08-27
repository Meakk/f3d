#include "F3DJavaBindings.h"

#include <app_f3d_F3D_VideoEncoder.h>
#include <app_f3d_F3D_VideoFrame.h>
#include <app_f3d_F3D_VideoPacket.h>

#include <video_encoder.h>

#include <map>

namespace
{
std::map<jlong, jobject> g_packetListeners;
}

extern "C"
{
  // VideoFrame
  JNIEXPORT void JAVA_BIND(VideoFrame, nativeDestroy)(JNIEnv*, jclass, jlong nativeAddress)
  {
    delete reinterpret_cast<f3d::video_frame*>(nativeAddress);
  }

  JNIEXPORT jobject JAVA_BIND(VideoFrame, setTimestamp)(JNIEnv* env, jobject self, jlong timestamp)
  {
    GetVideoFrame(env, self)->setTimestamp(timestamp);
    return self;
  }

  // VideoPacket
  JNIEXPORT jbyteArray JAVA_BIND(VideoPacket, getPacketData)(JNIEnv* env, jobject self)
  {
    f3d::video_packet* packet = GetVideoPacket(env, self);
    jsize size = static_cast<jsize>(packet->getPacketSize());

    jbyteArray result = env->NewByteArray(size);
    if (size > 0)
    {
      env->SetByteArrayRegion(result, 0, size, reinterpret_cast<jbyte*>(packet->getPacketData()));
    }
    return result;
  }

  JNIEXPORT jlong JAVA_BIND(VideoPacket, getTimestamp)(JNIEnv* env, jobject self)
  {
    return GetVideoPacket(env, self)->getTimestamp();
  }

  JNIEXPORT jboolean JAVA_BIND(VideoPacket, isKeyFrame)(JNIEnv* env, jobject self)
  {
    return GetVideoPacket(env, self)->isKeyFrame() ? JNI_TRUE : JNI_FALSE;
  }

  // VideoEncoder
  JNIEXPORT jlong JAVA_BIND(VideoEncoder, nativeCreate)(JNIEnv* env, jclass, jint codec, jint width,
    jint height, jdouble frameRate, jint compression, jboolean lowLatency)
  {
    f3d::video_encoder::params params;
    params.Codec = static_cast<f3d::video_encoder::codec>(codec);
    params.Width = width;
    params.Height = height;
    params.FrameRate = frameRate;
    params.Compression = static_cast<f3d::video_encoder::compression>(compression);
    params.LowLatency = lowLatency == JNI_TRUE;

    try
    {
      return reinterpret_cast<jlong>(new f3d::video_encoder(params));
    }
    catch (const f3d::video_encoder::codec_exception& e)
    {
      F3DThrowJavaException(env, "app/f3d/F3D/VideoEncoder$CodecException", e.what());
      return 0;
    }
  }

  JNIEXPORT void JAVA_BIND(VideoEncoder, nativeDestroy)(JNIEnv* env, jclass, jlong nativeAddress)
  {
    auto it = g_packetListeners.find(nativeAddress);
    if (it != g_packetListeners.end())
    {
      env->DeleteGlobalRef(it->second);
      g_packetListeners.erase(it);
    }
    delete reinterpret_cast<f3d::video_encoder*>(nativeAddress);
  }

  JNIEXPORT jint JAVA_BIND(VideoEncoder, getWidth)(JNIEnv* env, jobject self)
  {
    return GetVideoEncoder(env, self)->getWidth();
  }

  JNIEXPORT jint JAVA_BIND(VideoEncoder, getHeight)(JNIEnv* env, jobject self)
  {
    return GetVideoEncoder(env, self)->getHeight();
  }

  JNIEXPORT jobject JAVA_BIND(VideoEncoder, listen)(JNIEnv* env, jobject self, jobject listener)
  {
    f3d::video_encoder* encoder = GetVideoEncoder(env, self);

    JniLocalRef<jclass> cls(env, env->GetObjectClass(self));
    jlong nativeAddress = env->GetLongField(self, env->GetFieldID(cls, "mNativeAddress", "J"));

    JavaVM* jvm = nullptr;
    env->GetJavaVM(&jvm);

    auto it = g_packetListeners.find(nativeAddress);
    if (it != g_packetListeners.end())
    {
      env->DeleteGlobalRef(it->second);
    }
    g_packetListeners[nativeAddress] = env->NewGlobalRef(listener);

    encoder->listen(
      [=](const f3d::video_packet& packet)
      {
        JNIEnv* threadEnv = nullptr;
#ifdef __ANDROID__
        if (jvm->AttachCurrentThread(&threadEnv, nullptr) != JNI_OK)
#else
        if (jvm->AttachCurrentThread(reinterpret_cast<void**>(&threadEnv), nullptr) != JNI_OK)
#endif
        {
          return;
        }

        auto listenerIt = g_packetListeners.find(nativeAddress);
        if (listenerIt == g_packetListeners.end())
        {
          jvm->DetachCurrentThread();
          return;
        }
        jobject listenerObj = listenerIt->second;

        // Scoped so all JniLocalRef destructors run before DetachCurrentThread
        {
          JniLocalRef<jclass> packetClass(
            threadEnv, threadEnv->FindClass("app/f3d/F3D/VideoPacket"));
          jmethodID packetCtor = threadEnv->GetMethodID(packetClass, "<init>", "(J)V");
          JniLocalRef<jobject> packetObj(threadEnv,
            threadEnv->NewObject(packetClass, packetCtor, reinterpret_cast<jlong>(&packet)));

          JniLocalRef<jclass> listenerClass(threadEnv, threadEnv->GetObjectClass(listenerObj));
          jmethodID executeMethod =
            threadEnv->GetMethodID(listenerClass, "execute", "(Lapp/f3d/F3D/VideoPacket;)V");
          threadEnv->CallVoidMethod(listenerObj, executeMethod, packetObj.get());
        }

        jvm->DetachCurrentThread();
      });

    return self;
  }

  JNIEXPORT jboolean JAVA_BIND(VideoEncoder, submit)(JNIEnv* env, jobject self, jobject frame)
  {
    f3d::video_encoder* encoder = GetVideoEncoder(env, self);
    f3d::video_frame* cppFrame = GetVideoFrame(env, frame);

    try
    {
      return encoder->submit(*cppFrame) ? JNI_TRUE : JNI_FALSE;
    }
    catch (const f3d::video_encoder::transport_exception& e)
    {
      F3DThrowJavaException(env, "app/f3d/F3D/VideoEncoder$TransportException", e.what());
      return JNI_FALSE;
    }
  }

  JNIEXPORT jobject JAVA_BIND(VideoEncoder, flush)(JNIEnv* env, jobject self)
  {
    GetVideoEncoder(env, self)->flush();
    return self;
  }
}
