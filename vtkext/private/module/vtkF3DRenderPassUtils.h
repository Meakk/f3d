/**
 * Utilities shared by the render passes of F3D.
 */

#ifndef vtkF3DRenderPassUtils_h
#define vtkF3DRenderPassUtils_h

#include <vtkRenderState.h>
#include <vtkRenderer.h>
#include <vtkOpenGLState.h>
#include <vtkOpenGLFramebufferObject.h>
#include <vtk_glad.h>

#include <cmath>

namespace vtkF3DRenderPassUtils
{
/**
 * Compute the size of the framebuffers of a render pass, which is the size of the framebuffer of
 * the render state if any or the size of the renderer otherwise, scaled by the resolution scale
 * used for supersampling.
 */
inline void ComputeRenderSize(
  const vtkRenderState* s, double resolutionScale, int& width, int& height)
{
  if (s->GetFrameBuffer() == nullptr)
  {
    int x;
    int y;
    s->GetRenderer()->GetTiledSizeAndOrigin(&width, &height, &x, &y);
  }
  else
  {
    int size[2];
    s->GetWindowSize(size);
    width = size[0];
    height = size[1];
  }

  width = static_cast<int>(std::lround(width * resolutionScale));
  height = static_cast<int>(std::lround(height * resolutionScale));
}

enum DepthTest
{
  DepthTestNone,
  DepthTestLEqual
};

enum Blend
{
  BlendNone,
  BlendStraight,
  BlendPremultiplied
};

struct FramebufferOptions
{
  int Width = -1;
  int Height = -1;
  DepthTest DepthTestMode = DepthTestNone;
  Blend BlendMode = BlendNone;
  bool ClearColor = false;
  bool ClearDepth = false;
};

struct ScopedFramebuffer
{
  ScopedFramebuffer(vtkOpenGLFramebufferObject* fbo, vtkOpenGLState* ostate, FramebufferOptions options)
    : State(ostate)
  {
    // TODO: possible improvements:
    // - Do not push/pop and just bind the FBO directly
    // - StartNonOrtho checks too many things
    this->State->PushFramebufferBindings();
    fbo->StartNonOrtho(options.Width, options.Height);

    // Set depth test mode
    switch (options.DepthTestMode)
    {
      case DepthTestNone:
        this->State->vtkglDisable(GL_DEPTH_TEST);
        break;
      case DepthTestLEqual:
        this->State->vtkglEnable(GL_DEPTH_TEST);
        this->State->vtkglDepthFunc(GL_LEQUAL);
        break;
    }

    // Set blend mode
    switch (options.BlendMode)
    {
      case BlendNone:
        this->State->vtkglDisable(GL_BLEND);
        break;
      case BlendStraight:
        this->State->vtkglEnable(GL_BLEND);
        this->State->vtkglBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        break;
      case BlendPremultiplied:
        this->State->vtkglEnable(GL_BLEND);
        this->State->vtkglBlendFuncSeparate(GL_ONE, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        break;
    }

    unsigned int clearMask = 0;

    if (options.ClearColor)
    {
      clearMask |= GL_COLOR_BUFFER_BIT;
    }
    if (options.ClearDepth)
    {
      clearMask |= GL_DEPTH_BUFFER_BIT;
    }

    if (clearMask != 0)
    {
      this->State->vtkglClear(clearMask);
    }

    this->State->vtkglDisable(GL_SCISSOR_TEST);
    this->State->vtkglDisable(GL_STENCIL_TEST);

    ostate->vtkglViewport(0, 0, options.Width, options.Height);
  }

  ~ScopedFramebuffer()
  {
    this->State->PopFramebufferBindings();
  }

  ScopedFramebuffer(const ScopedFramebuffer&) = delete;
  ScopedFramebuffer& operator=(const ScopedFramebuffer&) = delete;

private:
  vtkOpenGLState* State;
};

}

#endif
