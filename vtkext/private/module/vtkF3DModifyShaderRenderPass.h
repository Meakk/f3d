/**
 * @class   vtkF3DModifyShaderRenderPass
 * @brief   Base class for the render passes modifying the shaders of the props they render.
 *
 * vtkF3DModifyShaderRenderPass registers itself on the props of the render state, renders its
 * delegate pass and unregisters itself. While registered, the shader replacement methods of the
 * pass (PreReplaceShaderValues, PostReplaceShaderValues, ...) are applied by the mappers of the
 * props, including when they are rendered by the delegate pass.
 * Subclasses implement the shader replacement methods and can override Render() to do some work
 * before calling the superclass implementation.
 *
 * @sa
 * vtkOpenGLRenderPass
 */

#ifndef vtkF3DModifyShaderRenderPass_h
#define vtkF3DModifyShaderRenderPass_h

#include <vtkOpenGLRenderPass.h>
#include <vtkSmartPointer.h>

class vtkF3DModifyShaderRenderPass : public vtkOpenGLRenderPass
{
public:
  static vtkF3DModifyShaderRenderPass* New();
  vtkTypeMacro(vtkF3DModifyShaderRenderPass, vtkOpenGLRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Register the pass on the props, render the delegate pass and unregister the pass.
   */
  void Render(const vtkRenderState* s) override;

  /**
   * Release the graphics resources of the delegate pass.
   */
  void ReleaseGraphicsResources(vtkWindow* w) override;

  ///@{
  /**
   * Set/Get the pass rendering the props with the modified shaders.
   */
  void SetDelegatePass(vtkRenderPass* delegate);
  vtkRenderPass* GetDelegatePass() const { return this->DelegatePass; }
  ///@}

  vtkF3DModifyShaderRenderPass(const vtkF3DModifyShaderRenderPass&) = delete;
  void operator=(const vtkF3DModifyShaderRenderPass&) = delete;

protected:
  vtkF3DModifyShaderRenderPass() = default;
  ~vtkF3DModifyShaderRenderPass() override = default;

  vtkSmartPointer<vtkRenderPass> DelegatePass;
};

#endif
