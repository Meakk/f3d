#include "vtkF3DMainRenderPass.h"

#include "vtkF3DHexagonalBokehBlurPass.h"
#include "vtkF3DImporter.h"
#include "vtkF3DOpenGLGridMapper.h"
#include "vtkF3DPolyDataMapper.h"
#include "vtkF3DReflectionRenderPass.h"

#include <vtkActor.h>
#include <vtkInformation.h>
#include <vtkObjectFactory.h>
#include <vtkOpaquePass.h>
#include <vtkOpenGLError.h>
#include <vtkOpenGLFramebufferObject.h>
#include <vtkOpenGLRenderWindow.h>
#include <vtkOpenGLState.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProp3D.h>
#include <vtkProperty.h>
#include <vtkRenderState.h>
#include <vtkRenderer.h>
#include <vtkSkybox.h>
#include <vtkTextureObject.h>

#include <cmath>

vtkStandardNewMacro(vtkF3DMainRenderPass);

//------------------------------------------------------------------------------
vtkF3DMainRenderPass::vtkF3DMainRenderPass()
  : ColorTexture(vtkSmartPointer<vtkTextureObject>::New())
{
}

//------------------------------------------------------------------------------
vtkF3DMainRenderPass::~vtkF3DMainRenderPass() = default;

//------------------------------------------------------------------------------
void vtkF3DMainRenderPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ColorTexture: " << this->ColorTexture << "\n";
}

//------------------------------------------------------------------------------
void vtkF3DMainRenderPass::SetDelegatePass(vtkRenderPass* delegate)
{
  if (this->DelegatePass != delegate)
  {
    this->DelegatePass = delegate;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkF3DMainRenderPass::SetReflectionPass(vtkF3DReflectionRenderPass* reflectionPass)
{
  if (this->ReflectionPass != reflectionPass)
  {
    this->ReflectionPass = reflectionPass;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkF3DMainRenderPass::FilterProps(const vtkRenderState* s)
{
  this->BackgroundProps.clear();
  this->MainOnTopProps.clear();
  this->MainProps.clear();

  vtkProp* const* props = s->GetPropArray();
  for (int i = 0; i < s->GetPropArrayCount(); i++)
  {
    vtkProp* prop = props[i];
    if (vtkSkybox::SafeDownCast(prop))
    {
      this->BackgroundProps.push_back(prop);
    }
    else if (vtkProp3D::SafeDownCast(prop))
    {
      // armature
      vtkInformation* info = prop->GetPropertyKeys();
      if (info && info->Has(vtkF3DImporter::ACTOR_IS_ARMATURE()))
      {
        if (this->ArmatureVisible)
        {
          this->MainOnTopProps.push_back(prop);
        }
      }
      else
      {
        vtkActor* actor = vtkActor::SafeDownCast(prop);

        if (actor)
        {
          vtkPolyDataMapper* polyMapper = vtkPolyDataMapper::SafeDownCast(actor->GetMapper());
          if (polyMapper)
          {
#if F3D_MODULE_RAYTRACING
            // Enable linear colorspace conversion during raytracing on PBR actors.
            if (auto* mapper = vtkF3DPolyDataMapper::SafeDownCast(polyMapper))
            {
              mapper->SetUseLinearColorSpace(
                this->UseRaytracing && actor->GetProperty()->GetInterpolation() == VTK_PBR);
            }
#endif
            polyMapper->SetVBOShiftScaleMethod(
              vtkPolyDataMapper::ShiftScaleMethodType::DISABLE_SHIFT_SCALE);

            vtkPolyData* input =
              polyMapper->GetNumberOfInputPorts() > 0 ? polyMapper->GetInput() : nullptr;

            if (input && input->GetNumberOfPoints() == 0)
            {
              // skip rendering if no point
              continue;
            }
          }
        }

        this->MainProps.push_back(prop);
      }
    }
  }
}

//------------------------------------------------------------------------------
void vtkF3DMainRenderPass::SetDepthTexture(vtkTextureObject* depthTexture)
{
  if (this->DepthTexture != depthTexture)
  {
    this->DepthTexture = depthTexture;
    this->Modified();
  }
}

//------------------------------------------------------------------------------
void vtkF3DMainRenderPass::ReleaseGraphicsResources(vtkWindow* w)
{
  this->Superclass::ReleaseGraphicsResources(w);

  this->FrameBufferObject = nullptr;
  this->ColorTexture->ReleaseGraphicsResources(w);
}

//------------------------------------------------------------------------------
void vtkF3DMainRenderPass::InitializeResources(
  vtkOpenGLRenderWindow* renWin, int width, int height)
{
  assert(this->DepthTexture);

  this->ColorTexture->SetContext(renWin);
  if (!this->ColorTexture->GetHandle())
  {
    this->ColorTexture->SetInternalFormat(GL_RGBA32F);
    this->ColorTexture->SetDataType(GL_FLOAT);
    this->ColorTexture->Create2D(width, height, 4, VTK_UNSIGNED_CHAR, false);
  }
  this->ColorTexture->Resize(width, height);

  if (!this->FrameBufferObject)
  {
    this->FrameBufferObject = vtkSmartPointer<vtkOpenGLFramebufferObject>::New();
  }
  this->FrameBufferObject->SetContext(renWin);

  const vtkNew<vtkOpaquePass> bgP;
  if (this->UseBlurBackground)
  {
    vtkNew<vtkF3DHexagonalBokehBlurPass> blurP;
    blurP->SetCircleOfConfusionRadius(this->CircleOfConfusionRadius);
    blurP->SetDelegatePass(bgP);
    this->BackgroundPass = blurP;
  }
  else
  {
    this->BackgroundPass = bgP;
  }
}

//------------------------------------------------------------------------------
void vtkF3DMainRenderPass::Render(const vtkRenderState* s)
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

  const vtkOpenGLState::ScopedglEnableDisable bsaver(ostate, GL_BLEND);
  const vtkOpenGLState::ScopedglEnableDisable dsaver(ostate, GL_DEPTH_TEST);

  int width = 0;
  int height = 0;
  int x = 0;
  int y = 0;
  if (s->GetFrameBuffer() == nullptr)
  {
    r->GetTiledSizeAndOrigin(&width, &height, &x, &y);
  }
  else
  {
    int size[2];
    s->GetWindowSize(size);
    width = size[0];
    height = size[1];
  }

  width = static_cast<int>(std::lround(width * this->ResolutionScale));
  height = static_cast<int>(std::lround(height * this->ResolutionScale));

  this->InitializeResources(renWin, width, height);

  this->FilterProps(s);

  if (this->ReflectionPass)
  {
    // the reflection result is used when rendering the main props so it must be rendered before
    vtkRenderState reflState(r);
    reflState.SetPropArrayAndCount(
      this->MainProps.data(), static_cast<int>(this->MainProps.size()));
    reflState.SetFrameBuffer(s->GetFrameBuffer());

    this->ReflectionPass->Render(&reflState);
  }

  // StartNonOrtho does not set the viewport, which can be larger than the window with supersampling
  const vtkOpenGLState::ScopedglViewport viewportSaver(ostate);
  const vtkOpenGLState::ScopedglScissor scissorSaver(ostate);

  ostate->PushFramebufferBindings();

  this->FrameBufferObject->Bind();
  this->FrameBufferObject->AddColorAttachment(0, this->ColorTexture);
  this->FrameBufferObject->AddDepthAttachment(this->DepthTexture);
  this->FrameBufferObject->StartNonOrtho(width, height);
  ostate->vtkglViewport(0, 0, width, height);
  ostate->vtkglScissor(0, 0, width, height);

  ostate->vtkglEnable(GL_BLEND);
  ostate->vtkglBlendFuncSeparate(
    GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  // do not clear the depth buffer, it comes from the prepass
  ostate->vtkglClearColor(0.0, 0.0, 0.0, 0.0);
  ostate->vtkglClear(GL_COLOR_BUFFER_BIT);

  vtkRenderState s2(s->GetRenderer());
  s2.SetFrameBuffer(this->FrameBufferObject);

  // TODO: if TAA only?
  ostate->vtkglDisable(GL_DEPTH_TEST);

  // render skybox
  // will use the depth buffer from the prepass to limit overdraw
  // must be run first for proper compositing with the main translucent props
  s2.SetPropArrayAndCount(this->BackgroundProps.data(), static_cast<int>(this->BackgroundProps.size()));
  this->BackgroundPass->Render(&s2);
  this->NumberOfRenderedProps += this->BackgroundPass->GetNumberOfRenderedProps();

  ostate->vtkglEnable(GL_DEPTH_TEST);
  ostate->vtkglEnable(GL_BLEND);

  // render main props
  s2.SetPropArrayAndCount(this->MainProps.data(), static_cast<int>(this->MainProps.size()));
  this->DelegatePass->Render(&s2);
  this->NumberOfRenderedProps += this->DelegatePass->GetNumberOfRenderedProps();

  // render "on top" props (depth test disabled)
  ostate->vtkglDisable(GL_DEPTH_TEST);
  for (vtkProp* prop : this->MainOnTopProps)
  {
    prop->RenderOpaqueGeometry(r);
    this->NumberOfRenderedProps++;
  }

  ostate->PopFramebufferBindings();

  vtkOpenGLCheckErrorMacro("failed after Render");
}
