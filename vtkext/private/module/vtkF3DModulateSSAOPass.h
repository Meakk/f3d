/**
 * @class   vtkF3DModulateSSAOPass
 * @brief   Render pass modulating the color of props by the baked ambient occlusion.
 *
 * vtkF3DModulateSSAOPass modifies the fragment shaders of the props rendered by its delegate pass
 * to multiply the final fragment color by the ambient occlusion texture baked by
 * vtkF3DBakeSSAOPass.
 * The ambient occlusion is fetched using the fragment coordinates, so the framebuffer being
 * rendered into has to have the same size as the ambient occlusion texture.
 *
 * The delegate pass is expected to render the opaque props.
 * The texture is not owned by the pass, it should be set before each render.
 *
 * @sa
 * vtkF3DModifyShaderRenderPass vtkF3DBakeSSAOPass
 */

#ifndef vtkF3DModulateSSAOPass_h
#define vtkF3DModulateSSAOPass_h

#include "vtkF3DModifyShaderRenderPass.h"

class vtkTextureObject;

class vtkF3DModulateSSAOPass : public vtkF3DModifyShaderRenderPass
{
public:
  static vtkF3DModulateSSAOPass* New();
  vtkTypeMacro(vtkF3DModulateSSAOPass, vtkF3DModifyShaderRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Make the ambient occlusion texture available to the shaders, then render the delegate pass.
   */
  void Render(const vtkRenderState* s) override;

  ///@{
  /**
   * Set/Get the single channel ambient occlusion texture, 1 meaning no occlusion.
   * The delegate pass is rendered without modulation if it is not set.
   */
  void SetSSAOTexture(vtkTextureObject* texture)
  {
    this->SSAOTexture = texture;
  }
  vtkTextureObject* GetSSAOTexture() const
  {
    return this->SSAOTexture;
  }
  ///@}

  /**
   * Multiply the final color by the ambient occlusion.
   */
  bool PostReplaceShaderValues(std::string& vertexShader, std::string& geometryShader,
    std::string& fragmentShader, vtkAbstractMapper* mapper, vtkProp* prop) override;

  /**
   * Bind the ambient occlusion texture.
   */
  bool SetShaderParameters(vtkShaderProgram* program, vtkAbstractMapper* mapper, vtkProp* prop,
    vtkOpenGLVertexArrayObject* VAO = nullptr) override;

  vtkF3DModulateSSAOPass(const vtkF3DModulateSSAOPass&) = delete;
  void operator=(const vtkF3DModulateSSAOPass&) = delete;

protected:
  vtkF3DModulateSSAOPass() = default;
  ~vtkF3DModulateSSAOPass() override = default;

  // the texture is owned by vtkF3DBakeSSAOPass
  vtkTextureObject* SSAOTexture = nullptr;
};

#endif
