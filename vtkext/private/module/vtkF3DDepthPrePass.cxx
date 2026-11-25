#include "vtkF3DDepthPrePass.h"

#include <vtkActor.h>
#include <vtkCameraPass.h>
#include <vtkMapper.h>
#include <vtkObjectFactory.h>
#include <vtkOpenGLError.h>
#include <vtkOpenGLFramebufferObject.h>
#include <vtkOpenGLRenderWindow.h>
#include <vtkOpenGLState.h>
#include <vtkPointGaussianMapper.h>
#include <vtkRenderState.h>
#include <vtkRenderer.h>
#include <vtkShaderProgram.h>
#include <vtkTextureObject.h>

vtkStandardNewMacro(vtkF3DDepthPrePass);

namespace
{
const char* const OCTAHEDRAL_ENCODE =
  "vec2 f3dOctahedralEncode(vec3 n)\n"
  "{\n"
  "  n /= (abs(n.x) + abs(n.y) + abs(n.z));\n"
  "  vec2 e = n.xy;\n"
  "  if (n.z < 0.0)\n"
  "  {\n"
  "    e = (1.0 - abs(e.yx)) * vec2(n.x >= 0.0 ? 1.0 : -1.0, n.y >= 0.0 ? 1.0 : -1.0);\n"
  "  }\n"
  "  return e * 0.5 + 0.5;\n"
  "}\n";
}

//------------------------------------------------------------------------------
vtkF3DDepthPrePass::vtkF3DDepthPrePass() = default;

//------------------------------------------------------------------------------
vtkF3DDepthPrePass::~vtkF3DDepthPrePass() = default;

//------------------------------------------------------------------------------
void vtkF3DDepthPrePass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NormalTexture: " << this->NormalTexture << "\n";
  os << indent << "DepthTexture: " << this->DepthTexture << "\n";
}

//------------------------------------------------------------------------------
void vtkF3DDepthPrePass::SetDelegatePass(vtkRenderPass* delegate)
{
  if (this->DelegatePass != delegate)
  {
    this->DelegatePass = delegate;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkF3DDepthPrePass::InitializeResources(vtkOpenGLRenderWindow* renWin, int width, int height)
{
  if (!this->NormalTexture)
  {
    this->NormalTexture = vtkSmartPointer<vtkTextureObject>::New();
  }
  this->NormalTexture->SetContext(renWin);
  if (!this->NormalTexture->GetHandle())
  {
    this->NormalTexture->SetFormat(GL_RG);
#ifdef F3D_USE_GLES
    // RG16 is not available on GLES, use the half float equivalent
    this->NormalTexture->SetInternalFormat(GL_RG16F);
    this->NormalTexture->SetDataType(GL_FLOAT);
    const int dataType = VTK_FLOAT;
#else
    this->NormalTexture->SetInternalFormat(GL_RG16);
    this->NormalTexture->SetDataType(GL_UNSIGNED_SHORT);
    const int dataType = VTK_UNSIGNED_SHORT;
#endif
    this->NormalTexture->SetMinificationFilter(vtkTextureObject::Nearest);
    this->NormalTexture->SetMagnificationFilter(vtkTextureObject::Nearest);
    this->NormalTexture->SetWrapS(vtkTextureObject::ClampToEdge);
    this->NormalTexture->SetWrapT(vtkTextureObject::ClampToEdge);
    this->NormalTexture->Allocate2D(width, height, 2, dataType);
  }
  this->NormalTexture->Resize(width, height);

  if (!this->DepthTexture)
  {
    this->DepthTexture = vtkSmartPointer<vtkTextureObject>::New();
  }
  this->DepthTexture->SetContext(renWin);
  if (!this->DepthTexture->GetHandle())
  {
    this->DepthTexture->AllocateDepth(width, height, vtkTextureObject::Fixed24);
  }
  this->DepthTexture->Resize(width, height);

  if (!this->FrameBufferObject)
  {
    this->FrameBufferObject = vtkSmartPointer<vtkOpenGLFramebufferObject>::New();
  }
  this->FrameBufferObject->SetContext(renWin);
}

//------------------------------------------------------------------------------
void vtkF3DDepthPrePass::Render(const vtkRenderState* s)
{
  vtkOpenGLClearErrorMacro();

  this->NumberOfRenderedProps = 0;

  if (!this->DelegatePass)
  {
    vtkWarningMacro(<< " no delegate.");
    return;
  }

  vtkRenderer* r = s->GetRenderer();
  vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(r->GetRenderWindow());
  vtkOpenGLState* ostate = renWin->GetState();

  int width = 0;
  int height = 0;
  if (s->GetFrameBuffer() == nullptr)
  {
    int x;
    int y;
    r->GetTiledSizeAndOrigin(&width, &height, &x, &y);
  }
  else
  {
    int size[2];
    s->GetWindowSize(size);
    width = size[0];
    height = size[1];
  }

  this->InitializeResources(renWin, width, height);

  // only opaque actors go through this pass
  this->OpaqueProps.clear();
  vtkProp* const* props = s->GetPropArray();
  for (int i = 0; i < s->GetPropArrayCount(); i++)
  {
    vtkActor* actor = vtkActor::SafeDownCast(props[i]);
    if (actor && actor->GetVisibility() && actor->GetMapper() &&
      !vtkPointGaussianMapper::SafeDownCast(actor->GetMapper()) &&
      !actor->HasTranslucentPolygonalGeometry())
    {
      this->OpaqueProps.push_back(actor);
    }
  }

  vtkOpenGLFramebufferObject* fbo = this->FrameBufferObject;

  vtkRenderState s2(r);
  s2.SetPropArrayAndCount(this->OpaqueProps.data(), static_cast<int>(this->OpaqueProps.size()));
  s2.SetFrameBuffer(fbo);

  ostate->PushFramebufferBindings();

  fbo->Bind();
  fbo->AddColorAttachment(0, this->NormalTexture);
  fbo->ActivateDrawBuffer(0);
  fbo->AddDepthAttachment(this->DepthTexture);
  fbo->StartNonOrtho(width, height);

  {
    const vtkOpenGLState::ScopedglEnableDisable bsaver(ostate, GL_BLEND);
    const vtkOpenGLState::ScopedglEnableDisable dsaver(ostate, GL_DEPTH_TEST);
    const vtkOpenGLState::ScopedglDepthMask dmsaver(ostate);
    const vtkOpenGLState::ScopedglDepthFunc dfsaver(ostate);
    const vtkOpenGLState::ScopedglColorMask cmsaver(ostate);
    const vtkOpenGLState::ScopedglClearColor ccsaver(ostate);

    ostate->vtkglDisable(GL_BLEND);
    ostate->vtkglEnable(GL_DEPTH_TEST);
    ostate->vtkglDepthMask(GL_TRUE);
    ostate->vtkglDepthFunc(GL_LEQUAL);
    ostate->vtkglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

    ostate->vtkglClearColor(0.0, 0.0, 0.0, 0.0);
    ostate->vtkglClearDepth(1.0);
    ostate->vtkglClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // register this pass on the props so PostReplaceShaderValues is called for them
    this->PreRender(&s2);
    this->DelegatePass->Render(&s2);
    this->PostRender(&s2);

    this->NumberOfRenderedProps = this->DelegatePass->GetNumberOfRenderedProps();
  }

  ostate->PopFramebufferBindings();

  vtkOpenGLCheckErrorMacro("failed after Render");
}

//------------------------------------------------------------------------------
bool vtkF3DDepthPrePass::PostReplaceShaderValues(std::string&, std::string&,
  std::string& fragmentShader, vtkAbstractMapper*, vtkProp*)
{
  // vtkF3DRenderPass can be registered on the same props, only modify the shader when
  // the final color token is available
  const std::string outputToken = "//VTK::Picking::Impl";
  if (fragmentShader.find(outputToken) == std::string::npos ||
    fragmentShader.find("//VTK::System::Dec") == std::string::npos)
  {
    return true;
  }

  vtkShaderProgram::Substitute(fragmentShader, "//VTK::System::Dec",
    std::string("//VTK::System::Dec\n") + OCTAHEDRAL_ENCODE, false);

  // normalVCVSOutput is only defined when lighting is enabled
  const bool hasNormal = fragmentShader.find("vec3 normalVCVSOutput") != std::string::npos;
  const std::string normal = hasNormal ? "normalize(normalVCVSOutput)" : "vec3(0.0, 0.0, 1.0)";

  vtkShaderProgram::Substitute(fragmentShader, outputToken,
    "  gl_FragData[0] = vec4(f3dOctahedralEncode(" + normal + "), 0.0, 1.0);\n  " + outputToken +
      "\n",
    false);

  return true;
}

//------------------------------------------------------------------------------
void vtkF3DDepthPrePass::ReleaseGraphicsResources(vtkWindow* w)
{
  this->Superclass::ReleaseGraphicsResources(w);

  if (this->FrameBufferObject)
  {
    this->FrameBufferObject->ReleaseGraphicsResources(w);
  }
  if (this->NormalTexture)
  {
    this->NormalTexture->ReleaseGraphicsResources(w);
  }
  if (this->DepthTexture)
  {
    this->DepthTexture->ReleaseGraphicsResources(w);
  }
}
