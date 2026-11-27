#include "vtkF3DModulateSSAOPass.h"

#include <vtkObjectFactory.h>
#include <vtkRenderState.h>
#include <vtkShaderProgram.h>
#include <vtkTextureObject.h>

vtkStandardNewMacro(vtkF3DModulateSSAOPass);

// ----------------------------------------------------------------------------
void vtkF3DModulateSSAOPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "SSAOTexture: " << this->SSAOTexture << "\n";
}

// ----------------------------------------------------------------------------
void vtkF3DModulateSSAOPass::Render(const vtkRenderState* s)
{
  if (!this->SSAOTexture || !this->SSAOTexture->GetHandle())
  {
    // nothing to modulate with, do not modify the shaders
    this->NumberOfRenderedProps = 0;
    if (this->DelegatePass)
    {
      this->DelegatePass->Render(s);
      this->NumberOfRenderedProps = this->DelegatePass->GetNumberOfRenderedProps();
    }
    return;
  }

  // the texture unit has to stay reserved while the props are rendered
  this->SSAOTexture->Activate();
  this->Superclass::Render(s);
  this->SSAOTexture->Deactivate();
}

// ----------------------------------------------------------------------------
bool vtkF3DModulateSSAOPass::PostReplaceShaderValues(
  std::string&, std::string&, std::string& fragmentShader, vtkAbstractMapper*, vtkProp*)
{
  // only the shaders of the mappers following the VTK templates can be modulated,
  // the picking token is located after all the color computations
  const std::string outputToken = "//VTK::Picking::Impl";
  if (fragmentShader.find(outputToken) == std::string::npos ||
    fragmentShader.find("//VTK::System::Dec") == std::string::npos)
  {
    return true;
  }

  vtkShaderProgram::Substitute(fragmentShader, "//VTK::System::Dec",
    "//VTK::System::Dec\n"
    "uniform sampler2D texSSAO;\n",
    false);

  vtkShaderProgram::Substitute(fragmentShader, outputToken,
    "  ivec2 ssaoPixel = min(ivec2(gl_FragCoord.xy), textureSize(texSSAO, 0) - 1);\n"
    "  gl_FragData[0].rgb *= texelFetch(texSSAO, ssaoPixel, 0).r;\n  " +
      outputToken + "\n",
    false);

  return true;
}

// ----------------------------------------------------------------------------
bool vtkF3DModulateSSAOPass::SetShaderParameters(
  vtkShaderProgram* program, vtkAbstractMapper*, vtkProp*, vtkOpenGLVertexArrayObject*)
{
  if (this->SSAOTexture && program->IsUniformUsed("texSSAO"))
  {
    program->SetUniformi("texSSAO", this->SSAOTexture->GetTextureUnit());
  }
  return true;
}
