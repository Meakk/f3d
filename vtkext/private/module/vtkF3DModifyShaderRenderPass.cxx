#include "vtkF3DModifyShaderRenderPass.h"

#include <vtkObjectFactory.h>
#include <vtkRenderState.h>

vtkStandardNewMacro(vtkF3DModifyShaderRenderPass);

// ----------------------------------------------------------------------------
void vtkF3DModifyShaderRenderPass::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "DelegatePass: " << this->DelegatePass << "\n";
}

// ----------------------------------------------------------------------------
void vtkF3DModifyShaderRenderPass::SetDelegatePass(vtkRenderPass* delegate)
{
  if (this->DelegatePass != delegate)
  {
    this->DelegatePass = delegate;
    this->Modified();
  }
}

// ----------------------------------------------------------------------------
void vtkF3DModifyShaderRenderPass::ReleaseGraphicsResources(vtkWindow* w)
{
  this->Superclass::ReleaseGraphicsResources(w);

  if (this->DelegatePass)
  {
    this->DelegatePass->ReleaseGraphicsResources(w);
  }
}

// ----------------------------------------------------------------------------
void vtkF3DModifyShaderRenderPass::Render(const vtkRenderState* s)
{
  this->NumberOfRenderedProps = 0;

  this->PreRender(s);

  if (this->DelegatePass)
  {
    this->DelegatePass->Render(s);
    this->NumberOfRenderedProps = this->DelegatePass->GetNumberOfRenderedProps();
  }
  else
  {
    vtkWarningMacro(<< " no delegate.");
  }

  this->PostRender(s);
}
