/**
 * @class   vtkF3DPostprocessRenderPass
 * @brief   Render pass drawing the rendered textures onto the current framebuffer.
 *
 * vtkF3DPostprocessRenderPass draws a full screen quad sampling the color texture on the
 * framebuffer of the render state, with blending and depth test disabled.
 * The depth and normal textures are optional and made available to the shader.
 * Textures are not owned by the pass, they should be set before each render.
 *
 * @sa
 * vtkRenderPass
 */

#ifndef vtkF3DPostprocessRenderPass_h
#define vtkF3DPostprocessRenderPass_h

#include <vtkRenderPass.h>

#include <memory>

class vtkOpenGLQuadHelper;
class vtkTextureObject;

class vtkF3DPostprocessRenderPass : public vtkRenderPass
{
public:
  static vtkF3DPostprocessRenderPass* New();
  vtkTypeMacro(vtkF3DPostprocessRenderPass, vtkRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Render(const vtkRenderState* s) override;

  void ReleaseGraphicsResources(vtkWindow* w) override;

  vtkSetMacro(ColorInputIsLinear, bool);
  vtkGetMacro(ColorInputIsLinear, bool);

  /**
   * Set the resolution scale of the input textures, if above 1 the color is downsampled (SSAA).
   */
  vtkSetMacro(ResolutionScale, double);

  vtkSetMacro(UseToneMapping, bool);
  vtkGetMacro(UseToneMapping, bool);

  ///@{
  /**
   * Set/Get the color texture to draw. Nothing is rendered without it.
   */
  void SetColorTexture(vtkTextureObject* texture) { this->ColorTexture = texture; }
  vtkTextureObject* GetColorTexture() const { return this->ColorTexture; }
  ///@}

  ///@{
  /**
   * Set/Get the optional depth texture.
   */
  void SetDepthTexture(vtkTextureObject* texture) { this->DepthTexture = texture; }
  vtkTextureObject* GetDepthTexture() const { return this->DepthTexture; }
  ///@}

  ///@{
  /**
   * Set/Get the optional normal texture.
   */
  void SetNormalTexture(vtkTextureObject* texture) { this->NormalTexture = texture; }
  vtkTextureObject* GetNormalTexture() const { return this->NormalTexture; }
  ///@}

  vtkF3DPostprocessRenderPass(const vtkF3DPostprocessRenderPass&) = delete;
  void operator=(const vtkF3DPostprocessRenderPass&) = delete;

protected:
  vtkF3DPostprocessRenderPass();
  ~vtkF3DPostprocessRenderPass() override;

  // the textures are owned by other passes
  vtkTextureObject* ColorTexture = nullptr;
  vtkTextureObject* DepthTexture = nullptr;
  vtkTextureObject* NormalTexture = nullptr;

  bool ColorInputIsLinear = true;
  bool UseToneMapping = false;
  double ResolutionScale = 1.0;

  std::shared_ptr<vtkOpenGLQuadHelper> QuadHelper;
};

#endif
