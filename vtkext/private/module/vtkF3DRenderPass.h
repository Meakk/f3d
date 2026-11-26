/**
 * @class   vtkF3DRenderPass
 * @brief   Implement the basic render passes.
 *
 * vtkF3DRenderPass implements the render pass chain of F3D.
 * It first runs a depth pre-pass writing the normals and depth of opaque actors.
 * Then it consists of two main passes. The first one renders the background.
 * The second pass renders the dataset with different options (raytracing, SSAO, depth peeling, ...)
 * Once the two passes are rendered into textures, a final shader is applied to combine the
 * background (and optionally blur it using Bokeh depth of field) and the dataset image.
 *
 * @sa
 * vtkOpenGLRenderPass
 */

#ifndef vtkF3DRenderPass_h
#define vtkF3DRenderPass_h

#include <vtkOpenGLQuadHelper.h>
#include <vtkOpenGLRenderPass.h>
#include <vtkSmartPointer.h>
#include <vtkTimeStamp.h>

#include <memory>

class vtkActor;
class vtkF3DDepthPrePass;
class vtkF3DPostprocessRenderPass;
class vtkF3DMainRenderPass;
class vtkF3DReflectionRenderPass;
class vtkDefaultPass;
class vtkTextureObject;
class vtkCamera;
class vtkInformationIntegerKey;
class vtkAbstractMapper;
class vtkPolyData;
class vtkMatrix4x4;
class vtkProp;

class vtkF3DRenderPass : public vtkOpenGLRenderPass
{
public:
  static vtkF3DRenderPass* New();
  vtkTypeMacro(vtkF3DRenderPass, vtkOpenGLRenderPass);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  void Render(const vtkRenderState* s) override;

  vtkSetMacro(ArmatureVisible, bool);
  vtkSetMacro(UseRaytracing, bool);
  vtkSetMacro(UseAnari, bool);
  vtkSetMacro(UseSSAOPass, bool);
  vtkSetMacro(UseBlurBackground, bool);
  vtkSetMacro(ForceOpaqueBackground, bool);
  vtkSetVector6Macro(Bounds, double);
  vtkSetMacro(CircleOfConfusionRadius, double);
  vtkSetMacro(RenderReflection, bool);
  vtkSetMacro(XRMode, bool);
  vtkSetMacro(RenderShadows, bool);
  vtkSetMacro(ShadowExponentialConstant, double);
  vtkSetMacro(ShadowResolution, int);
  vtkSetMacro(AnariLibrary, std::string);

  /**
   * Modify shader code for matcap/gamma
   */
  bool PreReplaceShaderValues(std::string& vertexShader, std::string& geometryShader,
    std::string& fragmentShader, vtkAbstractMapper* mapper, vtkProp* prop) override;
  bool PostReplaceShaderValues(std::string& vertexShader, std::string& geometryShader,
    std::string& fragmentShader, vtkAbstractMapper* mapper, vtkProp* prop) override;

  vtkF3DRenderPass(const vtkF3DRenderPass&) = delete;
  void operator=(const vtkF3DRenderPass&) = delete;

  static vtkInformationIntegerKey* RENDER_UI_ONLY();

protected:
  vtkF3DRenderPass() = default;
  ~vtkF3DRenderPass() override = default;

  void ReleaseGraphicsResources(vtkWindow* w) override;

  void Initialize(const vtkRenderState* s);

  bool ArmatureVisible = false;
  bool UseRaytracing = false;
  bool UseAnari = false;
  std::string AnariLibrary = "environment";
  bool UseSSAOPass = false;
  bool UseBlurBackground = false;
  bool ForceOpaqueBackground = false;
  bool RenderReflection = false;
  bool XRMode = false;
  bool RenderShadows = false;
  double ShadowExponentialConstant = 25.0;
  int ShadowResolution = 1024;

  double CircleOfConfusionRadius = 20.0;

  vtkSmartPointer<vtkDefaultPass> OverlayPass;
  vtkSmartPointer<vtkF3DPostprocessRenderPass> PostprocessPass;
  vtkSmartPointer<vtkF3DDepthPrePass> DepthPrePass;
  vtkSmartPointer<vtkF3DReflectionRenderPass> BakeReflectionPass;
  vtkSmartPointer<vtkF3DMainRenderPass> MainPass;

  // depth texture reused in many passes
  vtkSmartPointer<vtkTextureObject> DepthTexture;

  double Bounds[6] = {};

  vtkMTimeType InitializeTime = 0;

private:
  void ReplaceMatCapShader(std::string& fragmentShader, vtkActor* actor, vtkPolyData* polyData);
};

#endif
