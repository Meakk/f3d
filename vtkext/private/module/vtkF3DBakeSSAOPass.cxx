#include "vtkF3DBakeSSAOPass.h"

#include "vtkF3DBakeSSAOFS.h"

#include <vtkCamera.h>
#include <vtkMath.h>
#include <vtkMatrix4x4.h>
#include <vtkNew.h>
#include <vtkObjectFactory.h>
#include <vtkOpenGLError.h>
#include <vtkOpenGLFramebufferObject.h>
#include <vtkOpenGLQuadHelper.h>
#include <vtkOpenGLRenderUtilities.h>
#include <vtkOpenGLRenderWindow.h>
#include <vtkOpenGLShaderCache.h>
#include <vtkOpenGLState.h>
#include <vtkRenderState.h>
#include <vtkRenderer.h>
#include <vtkShaderProgram.h>
#include <vtkTextureObject.h>

#include <random>

vtkStandardNewMacro(vtkF3DBakeSSAOPass);

//------------------------------------------------------------------------------
vtkF3DBakeSSAOPass::vtkF3DBakeSSAOPass()
{
  // created early to be shared with the passes consuming it, allocated when rendering
  this->SSAOTexture = vtkSmartPointer<vtkTextureObject>::New();
}

//------------------------------------------------------------------------------
vtkF3DBakeSSAOPass::~vtkF3DBakeSSAOPass() = default;

//------------------------------------------------------------------------------
void vtkF3DBakeSSAOPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DepthTexture: " << this->DepthTexture << "\n";
  os << indent << "NormalTexture: " << this->NormalTexture << "\n";
  os << indent << "SSAOTexture: " << this->SSAOTexture << "\n";
  os << indent << "Radius: " << this->Radius << "\n";
  os << indent << "Bias: " << this->Bias << "\n";
  os << indent << "KernelSize: " << this->KernelSize << "\n";
}

//------------------------------------------------------------------------------
void vtkF3DBakeSSAOPass::ReleaseGraphicsResources(vtkWindow* w)
{
  this->Superclass::ReleaseGraphicsResources(w);

  if (this->SSAOQuadHelper)
  {
    this->SSAOQuadHelper->ReleaseGraphicsResources(w);
    this->SSAOQuadHelper = nullptr;
  }
  if (this->FrameBufferObject)
  {
    this->FrameBufferObject->ReleaseGraphicsResources(w);
  }
  if (this->SSAOTexture)
  {
    this->SSAOTexture->ReleaseGraphicsResources(w);
  }
}

//------------------------------------------------------------------------------
void vtkF3DBakeSSAOPass::ComputeKernel()
{
  std::uniform_real_distribution<float> randomFloats(0.0, 1.0);
  std::default_random_engine generator;

  this->Kernel.resize(3 * this->KernelSize);

  for (unsigned int i = 0; i < this->KernelSize; ++i)
  {
    float sample[3] = { randomFloats(generator) * 2.f - 1.f, randomFloats(generator) * 2.f - 1.f,
      randomFloats(generator) };

    // reject the sample if not in the hemisphere
    if (vtkMath::Norm(sample) > 1.0)
    {
      i--;
      continue;
    }

    // more samples closer to the point
    float scale = i / static_cast<float>(this->KernelSize);
    scale = 0.1f + 0.9f * scale * scale;
    vtkMath::MultiplyScalar(sample, scale);

    this->Kernel[3 * i] = sample[0];
    this->Kernel[3 * i + 1] = sample[1];
    this->Kernel[3 * i + 2] = sample[2];
  }
}

//------------------------------------------------------------------------------
void vtkF3DBakeSSAOPass::InitializeResources(vtkOpenGLRenderWindow* renWin, int width, int height)
{
  this->SSAOTexture->SetContext(renWin);
  if (!this->SSAOTexture->GetHandle())
  {
    this->SSAOTexture->SetFormat(GL_RED);
    this->SSAOTexture->SetInternalFormat(GL_R8);
    this->SSAOTexture->SetDataType(GL_UNSIGNED_BYTE);
    this->SSAOTexture->SetMinificationFilter(vtkTextureObject::Nearest);
    this->SSAOTexture->SetMagnificationFilter(vtkTextureObject::Nearest);
    this->SSAOTexture->SetWrapS(vtkTextureObject::ClampToEdge);
    this->SSAOTexture->SetWrapT(vtkTextureObject::ClampToEdge);
    this->SSAOTexture->Allocate2D(width, height, 1, VTK_UNSIGNED_CHAR);
  }
  this->SSAOTexture->Resize(width, height);

  if (!this->FrameBufferObject)
  {
    this->FrameBufferObject = vtkSmartPointer<vtkOpenGLFramebufferObject>::New();
  }
  this->FrameBufferObject->SetContext(renWin);
}

//------------------------------------------------------------------------------
void vtkF3DBakeSSAOPass::Render(const vtkRenderState* s)
{
  vtkOpenGLClearErrorMacro();

  this->NumberOfRenderedProps = 0;

  if (!this->DepthTexture || !this->NormalTexture)
  {
    vtkWarningMacro(<< " no depth or normal texture.");
    return;
  }

  vtkRenderer* r = s->GetRenderer();
  vtkOpenGLRenderWindow* renWin = vtkOpenGLRenderWindow::SafeDownCast(r->GetRenderWindow());
  vtkOpenGLState* ostate = renWin->GetState();

  const int width = this->DepthTexture->GetWidth();
  const int height = this->DepthTexture->GetHeight();

  this->InitializeResources(renWin, width, height);

  if (this->SSAOQuadHelper && this->ShaderBuildTime < this->GetMTime())
  {
    this->SSAOQuadHelper = nullptr;
  }

  if (!this->SSAOQuadHelper)
  {
    this->ComputeKernel();

    std::string ssaoShader = vtkF3DBakeSSAOFS;
    vtkShaderProgram::Substitute(ssaoShader, "//VTK::SSAO::Kernel",
      "#define KERNEL_SIZE " + std::to_string(this->KernelSize) +
        "\nuniform vec3 samples[KERNEL_SIZE];");

    this->SSAOQuadHelper = std::make_shared<vtkOpenGLQuadHelper>(renWin,
      vtkOpenGLRenderUtilities::GetFullScreenQuadVertexShader().c_str(), ssaoShader.c_str(), "");

    this->ShaderBuildTime.Modified();
  }

  vtkShaderProgram* ssaoProgram = this->SSAOQuadHelper->Program;
  if (!ssaoProgram || !ssaoProgram->GetCompiled())
  {
    vtkErrorMacro("Couldn't build the SSAO shader program.");
    return;
  }

  // GetProjectionTransformMatrix returns a new transposed matrix, as expected by the uniforms
  vtkSmartPointer<vtkMatrix4x4> projection;
  projection.TakeReference(r->GetActiveCamera()->GetProjectionTransformMatrix(r));
  vtkNew<vtkMatrix4x4> invProjection;
  vtkMatrix4x4::Invert(projection, invProjection);

  const vtkOpenGLState::ScopedglEnableDisable bsaver(ostate, GL_BLEND);
  const vtkOpenGLState::ScopedglEnableDisable dsaver(ostate, GL_DEPTH_TEST);
  const vtkOpenGLState::ScopedglViewport viewportSaver(ostate);
  const vtkOpenGLState::ScopedglScissor scissorSaver(ostate);

  ostate->vtkglDisable(GL_BLEND);
  ostate->vtkglDisable(GL_DEPTH_TEST);

  vtkOpenGLFramebufferObject* fbo = this->FrameBufferObject;

  ostate->PushFramebufferBindings();
  fbo->Bind();

  // ambient occlusion
  renWin->GetShaderCache()->ReadyShaderProgram(ssaoProgram);
  fbo->AddColorAttachment(0, this->SSAOTexture);
  fbo->ActivateDrawBuffer(0);
  fbo->StartNonOrtho(width, height);
  ostate->vtkglViewport(0, 0, width, height);
  ostate->vtkglScissor(0, 0, width, height);

  this->DepthTexture->Activate();
  this->NormalTexture->Activate();
  ssaoProgram->SetUniformi("texDepth", this->DepthTexture->GetTextureUnit());
  ssaoProgram->SetUniformi("texNormal", this->NormalTexture->GetTextureUnit());
  ssaoProgram->SetUniformi("texNoise", renWin->GetNoiseTextureUnit());
  ssaoProgram->SetUniform3fv("samples", this->KernelSize, this->Kernel.data());
  ssaoProgram->SetUniformf("kernelRadius", static_cast<float>(this->Radius));
  ssaoProgram->SetUniformf("kernelBias", static_cast<float>(this->Bias));
  ssaoProgram->SetUniformMatrix("matProjection", projection);
  ssaoProgram->SetUniformMatrix("matInvProjection", invProjection);

  this->SSAOQuadHelper->Render();

  this->NormalTexture->Deactivate();
  this->DepthTexture->Deactivate();

  fbo->RemoveColorAttachments(1);

  ostate->PopFramebufferBindings();

  vtkOpenGLCheckErrorMacro("failed after Render");
}
