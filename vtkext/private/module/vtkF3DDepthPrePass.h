/**
 * @class   vtkF3DDepthPrePass
 * @brief   Render pass writing octahedrally encoded view space normals and depth of opaque actors.
 *
 * vtkF3DDepthPrePass renders the opaque actors only into an internal framebuffer made of
 * a 24 bits depth texture and a RG16 color texture.
 * The color texture contains the view space normals, octahedrally encoded.
 * Depth test and depth write are enabled and blending is disabled, so a pixel is simply replaced
 * if it passes the depth test.
 * Actors without lighting have no normal available, a normal facing the camera is written for them.
 *
 * The resulting textures are not consumed yet, they are exposed with
 * GetNormalTexture() and GetDepthTexture().
 *
 * @sa
 * vtkOpenGLRenderPass
 */

#ifndef vtkF3DDepthPrePass_h
#define vtkF3DDepthPrePass_h

#include <vtkOpenGLRenderPass.h>
#include <vtkSmartPointer.h>

#include <vector>

class vtkCameraPass;
class vtkOpenGLFramebufferObject;
class vtkOpenGLRenderWindow;
class vtkProp;
class vtkRenderPass;
class vtkTextureObject;

class vtkF3DDepthPrePass : public vtkOpenGLRenderPass
{
public:
  static vtkF3DDepthPrePass* New();
  vtkTypeMacro(vtkF3DDepthPrePass, vtkOpenGLRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Render(const vtkRenderState* s) override;

  void ReleaseGraphicsResources(vtkWindow* w) override;

  /**
   * Set the pass rendering the opaque props into the framebuffer, it is wrapped into a camera pass.
   * It is typically a sequence of passes ending with a vtkOpaquePass.
   */
  void SetDelegatePass(vtkRenderPass* delegate);
  vtkRenderPass* GetDelegatePass() const { return this->DelegatePass; }

  /**
   * Get the RG16 texture containing the octahedrally encoded view space normals.
   */
  vtkTextureObject* GetNormalTexture() const { return this->NormalTexture; }

  /**
   * Get the 24 bits depth texture.
   */
  vtkTextureObject* GetDepthTexture() const { return this->DepthTexture; }

  /**
   * Replace the fragment shader output by the octahedrally encoded normal.
   */
  bool PostReplaceShaderValues(std::string& vertexShader, std::string& geometryShader,
    std::string& fragmentShader, vtkAbstractMapper* mapper, vtkProp* prop) override;

  vtkF3DDepthPrePass(const vtkF3DDepthPrePass&) = delete;
  void operator=(const vtkF3DDepthPrePass&) = delete;

protected:
  vtkF3DDepthPrePass();
  ~vtkF3DDepthPrePass() override;

  void InitializeResources(vtkOpenGLRenderWindow* renWin, int width, int height);

  vtkSmartPointer<vtkTextureObject> NormalTexture;
  vtkSmartPointer<vtkTextureObject> DepthTexture;
  vtkSmartPointer<vtkOpenGLFramebufferObject> FrameBufferObject;

  vtkSmartPointer<vtkRenderPass> DelegatePass;

  std::vector<vtkProp*> OpaqueProps;
};

#endif
