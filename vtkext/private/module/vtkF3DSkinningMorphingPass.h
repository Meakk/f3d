/**
 * @class   vtkF3DSkinningMorphingPass
 * @brief   Render pass handling the skinning and morphing of actors.
 *
 * vtkF3DSkinningMorphingPass maps the glTF skinning and morphing arrays to vertex attributes
 * and modifies the vertex shaders of the actors to apply the skinning and morphing.
 *
 * This pass is meant to be added in a sequence, before the passes rendering the props.
 * Once rendered, it stays registered on the props of the render state, so all the following passes
 * and frames apply the skinning and morphing, until ReleaseGraphicsResources() is called.
 * The same instance can be used in several sequences.
 *
 * @sa
 * vtkOpenGLRenderPass
 */

#ifndef vtkF3DSkinningMorphingPass_h
#define vtkF3DSkinningMorphingPass_h

#include <vtkOpenGLRenderPass.h>
#include <vtkSmartPointer.h>
#include <vtkWeakPointer.h>

#include <vector>

class vtkActor;
class vtkPolyData;
class vtkPolyDataMapper;

class vtkF3DSkinningMorphingPass : public vtkOpenGLRenderPass
{
public:
  static vtkF3DSkinningMorphingPass* New();
  vtkTypeMacro(vtkF3DSkinningMorphingPass, vtkOpenGLRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Map the skinning and morphing arrays of the props and register the pass on them.
   */
  void Render(const vtkRenderState* s) override;

  /**
   * Unregister the pass from all the props it was registered on.
   */
  void ReleaseGraphicsResources(vtkWindow* w) override;

  /**
   * Modify the vertex shader to apply skinning and morphing.
   */
  bool PreReplaceShaderValues(std::string& vertexShader, std::string& geometryShader,
    std::string& fragmentShader, vtkAbstractMapper* mapper, vtkProp* prop) override;
  bool PostReplaceShaderValues(std::string& vertexShader, std::string& geometryShader,
    std::string& fragmentShader, vtkAbstractMapper* mapper, vtkProp* prop) override;

  vtkF3DSkinningMorphingPass(const vtkF3DSkinningMorphingPass&) = delete;
  void operator=(const vtkF3DSkinningMorphingPass&) = delete;

protected:
  vtkF3DSkinningMorphingPass() = default;
  ~vtkF3DSkinningMorphingPass() override = default;

  /**
   * Register the pass on the prop, only if it is not already registered.
   */
  void PreRenderProp(vtkProp* prop) override;

private:
  void MapAttributes(vtkPolyDataMapper* polyMapper, vtkPolyData* input);
  void ReplaceSkinningMorphing(std::string& vertexShader, vtkActor* actor, vtkPolyData* polyData);

  std::vector<vtkWeakPointer<vtkProp>> RegisteredProps;
  int LightComplexity = 0;
};

#endif
