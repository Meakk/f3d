#include "vtkF3DPostprocessRenderPass.h"

#include "vtkF3DRenderer.h"
#include "vtkF3DPostprocessFS.h"

#include <vtkObjectFactory.h>
#include <vtkOpenGLError.h>
#include <vtkOpenGLQuadHelper.h>
#include <vtkOpenGLRenderUtilities.h>
#include <vtkOpenGLRenderWindow.h>
#include <vtkOpenGLShaderCache.h>
#include <vtkOpenGLState.h>
#include <vtkRenderState.h>
#include <vtkRenderer.h>
#include <vtkShaderProgram.h>
#include <vtkTextureObject.h>

#include <sstream>

vtkStandardNewMacro(vtkF3DPostprocessRenderPass);

//------------------------------------------------------------------------------
vtkF3DPostprocessRenderPass::vtkF3DPostprocessRenderPass() = default;

//------------------------------------------------------------------------------
vtkF3DPostprocessRenderPass::~vtkF3DPostprocessRenderPass() = default;

//------------------------------------------------------------------------------
void vtkF3DPostprocessRenderPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ColorTexture: " << this->ColorTexture << "\n";
  os << indent << "DepthTexture: " << this->DepthTexture << "\n";
  os << indent << "NormalTexture: " << this->NormalTexture << "\n";
}

//------------------------------------------------------------------------------
void vtkF3DPostprocessRenderPass::ReleaseGraphicsResources(vtkWindow* w)
{
  this->Superclass::ReleaseGraphicsResources(w);

  if (this->QuadHelper)
  {
    this->QuadHelper->ReleaseGraphicsResources(w);
    this->QuadHelper = nullptr;
  }
}

//------------------------------------------------------------------------------
void vtkF3DPostprocessRenderPass::Render(const vtkRenderState* s)
{
  vtkOpenGLClearErrorMacro();

  this->NumberOfRenderedProps = 0;

  if (!this->ColorTexture)
  {
    vtkWarningMacro(<< " no color texture.");
    return;
  }

  vtkRenderer* r = s->GetRenderer();
  vtkOpenGLRenderWindow* renWin = static_cast<vtkOpenGLRenderWindow*>(r->GetRenderWindow());
  vtkOpenGLState* ostate = renWin->GetState();
  vtkF3DRenderer* ren = vtkF3DRenderer::SafeDownCast(r);

  const vtkOpenGLState::ScopedglEnableDisable bsaver(ostate, GL_BLEND);
  const vtkOpenGLState::ScopedglEnableDisable dsaver(ostate, GL_DEPTH_TEST);

  if (!this->QuadHelper)
  {
    std::string fsShader = vtkF3DPostprocessFS;

    if (!this->ColorInputIsLinear)
    {
      vtkShaderProgram::Substitute(fsShader, "//VTK::ColorSpace::Impl",
        "  color.rgb = pow(color.rgb, vec3(2.2));\n");
    }

    this->QuadHelper = std::make_shared<vtkOpenGLQuadHelper>(renWin,
      vtkOpenGLRenderUtilities::GetFullScreenQuadVertexShader().c_str(), fsShader.c_str(), "");
  }
  else
  {
    renWin->GetShaderCache()->ReadyShaderProgram(this->QuadHelper->Program);
  }

  if (!this->QuadHelper->Program || !this->QuadHelper->Program->GetCompiled())
  {
    vtkErrorMacro("Couldn't build the full screen shader program.");
    return;
  }

  this->ColorTexture->Activate();
  this->QuadHelper->Program->SetUniformi("texColor", this->ColorTexture->GetTextureUnit());

  if (this->DepthTexture)
  {
    this->DepthTexture->Activate();
    this->QuadHelper->Program->SetUniformi("texDepth", this->DepthTexture->GetTextureUnit());
  }

  if (this->NormalTexture)
  {
    this->NormalTexture->Activate();
    this->QuadHelper->Program->SetUniformi("texNormal", this->NormalTexture->GetTextureUnit());
  }

  this->QuadHelper->Program->SetUniform2i("resolution", renWin->GetSize());

  if (!ren)
  {
    this->QuadHelper->Program->SetUniformf("time", 0);
  }
  else
  {
    this->QuadHelper->Program->SetUniformf("time", ren->GetTotalTime());
  }

  ostate->vtkglEnable(GL_BLEND);
  ostate->vtkglBlendFuncSeparate(
    GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

  ostate->vtkglDisable(GL_DEPTH_TEST);

  r->Clear();

  this->QuadHelper->Render();

  this->ColorTexture->Deactivate();
  if (this->DepthTexture)
  {
    this->DepthTexture->Deactivate();
  }
  if (this->NormalTexture)
  {
    this->NormalTexture->Deactivate();
  }

  vtkOpenGLCheckErrorMacro("failed after Render");
}
