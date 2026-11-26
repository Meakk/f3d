/**
 * @class   vtkF3DBakeSSAOPass
 * @brief   Render pass baking the screen space ambient occlusion from the depth pre-pass buffers.
 *
 * vtkF3DBakeSSAOPass computes the screen space ambient occlusion (SSAO) from the depth texture and
 * the octahedrally encoded view space normal texture written by vtkF3DDepthPrePass.
 * The occlusion is computed in a full screen pass. The result is a single channel texture,
 * 1 meaning no occlusion, available with GetSSAOTexture() and has the same size as the input
 * textures.
 * It is meant to be consumed by vtkF3DModulateSSAOPass, nothing is drawn in the current
 * framebuffer.
 * The input textures are not owned by the pass, they should be set before each render.
 *
 * @sa
 * vtkF3DDepthPrePass vtkF3DModulateSSAOPass
 */

#ifndef vtkF3DBakeSSAOPass_h
#define vtkF3DBakeSSAOPass_h

#include <vtkRenderPass.h>
#include <vtkSmartPointer.h>
#include <vtkTimeStamp.h>

#include <memory>
#include <vector>

class vtkOpenGLFramebufferObject;
class vtkOpenGLQuadHelper;
class vtkOpenGLRenderWindow;
class vtkTextureObject;

class vtkF3DBakeSSAOPass : public vtkRenderPass
{
public:
  static vtkF3DBakeSSAOPass* New();
  vtkTypeMacro(vtkF3DBakeSSAOPass, vtkRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Compute the ambient occlusion using the active camera of the renderer of the state.
   */
  void Render(const vtkRenderState* s) override;

  void ReleaseGraphicsResources(vtkWindow* w) override;

  ///@{
  /**
   * Set/Get the depth texture, as written by vtkF3DDepthPrePass.
   */
  void SetDepthTexture(vtkTextureObject* texture)
  {
    this->DepthTexture = texture;
  }
  vtkTextureObject* GetDepthTexture() const
  {
    return this->DepthTexture;
  }
  ///@}

  ///@{
  /**
   * Set/Get the texture containing the octahedrally encoded view space normals,
   * as written by vtkF3DDepthPrePass.
   */
  void SetNormalTexture(vtkTextureObject* texture)
  {
    this->NormalTexture = texture;
  }
  vtkTextureObject* GetNormalTexture() const
  {
    return this->NormalTexture;
  }
  ///@}

  /**
   * Get the single channel texture containing the ambient occlusion.
   */
  vtkTextureObject* GetSSAOTexture() const
  {
    return this->SSAOTexture;
  }

  ///@{
  /**
   * Set/Get the radius of the sampling hemisphere, in world units.
   * Default is 0.5.
   */
  vtkSetMacro(Radius, double);
  vtkGetMacro(Radius, double);
  ///@}

  ///@{
  /**
   * Set/Get the bias used when comparing the depth of the samples, in world units.
   * Default is 0.01.
   */
  vtkSetMacro(Bias, double);
  vtkGetMacro(Bias, double);
  ///@}

  ///@{
  /**
   * Set/Get the number of samples per pixel.
   * Default is 64.
   */
  vtkSetClampMacro(KernelSize, unsigned int, 1, 256);
  vtkGetMacro(KernelSize, unsigned int);
  ///@}

  vtkF3DBakeSSAOPass(const vtkF3DBakeSSAOPass&) = delete;
  void operator=(const vtkF3DBakeSSAOPass&) = delete;

protected:
  vtkF3DBakeSSAOPass();
  ~vtkF3DBakeSSAOPass() override;

  void InitializeResources(vtkOpenGLRenderWindow* renWin, int width, int height);
  void ComputeKernel();

  // the input textures are owned by the depth pre-pass
  vtkTextureObject* DepthTexture = nullptr;
  vtkTextureObject* NormalTexture = nullptr;

  vtkSmartPointer<vtkTextureObject> SSAOTexture;
  vtkSmartPointer<vtkOpenGLFramebufferObject> FrameBufferObject;

  std::shared_ptr<vtkOpenGLQuadHelper> SSAOQuadHelper;
  vtkTimeStamp ShaderBuildTime;

  std::vector<float> Kernel;
  unsigned int KernelSize = 64;
  double Radius = 0.5;
  double Bias = 0.01;
};

#endif
