/**
 * @class   vtkF3DFrameCapture
 * @brief   Frame capture from a texture.
 *
 * vtkF3DFrameCapture is a helper class to capture a texture and convert it to YUV planes.
 * It is used to capture frames for video encoding.
 */

#ifndef vtkF3DFrameCapture_h
#define vtkF3DFrameCapture_h

#include <vtkObject.h>
#include <vtkTextureObject.h>

#include <memory>

class vtkOpenGLRenderer;

class vtkF3DFrameCapture : public vtkObject
{
public:
  static vtkF3DFrameCapture* New();
  vtkTypeMacro(vtkF3DFrameCapture, vtkObject);

  /**
   * Set the input texture to capture. The texture must be a 2D texture with RGBA format.
   */
  vtkSetObjectMacro(InputTexture, vtkTextureObject);

  /**
   * Capture the current texture and fill the provided buffers with Y and UV planes.
   * The buffers must be allocated with the correct size:
   * - Y plane: width * height bytes
   * - UV plane: (width / 2) * (height / 2) bytes
   */
  void Capture(vtkOpenGLRenderer* ren, std::byte* y, std::byte* uv);

  /**
   * Release graphics resources.
   */
  void ReleaseGraphicsResources(vtkWindow* win);

  vtkF3DFrameCapture(const vtkF3DFrameCapture&) = delete;
  void operator=(const vtkF3DFrameCapture&) = delete;

protected:
  vtkF3DFrameCapture();
  ~vtkF3DFrameCapture() override;

private:
  struct Internals;
  std::unique_ptr<Internals> Pimpl;

  /**
   * Render the input texture to YUV planes.
   */
  void RenderYUV(vtkOpenGLRenderer* ren);

  vtkTextureObject* InputTexture = nullptr;
};

#endif
