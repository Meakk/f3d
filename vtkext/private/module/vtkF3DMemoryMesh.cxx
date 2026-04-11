#include "vtkF3DMemoryMesh.h"

#include "vtkDataArrayRange.h"
#include "vtkFloatArray.h"
#include "vtkIdTypeArray.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSMPTools.h"

#include <numeric>

vtkStandardNewMacro(vtkF3DMemoryMesh);

//------------------------------------------------------------------------------
vtkF3DMemoryMesh::vtkF3DMemoryMesh()
{
  this->SetNumberOfInputPorts(0);
}

//------------------------------------------------------------------------------
vtkF3DMemoryMesh::~vtkF3DMemoryMesh() = default;

//------------------------------------------------------------------------------
void vtkF3DMemoryMesh::SetPoints(vtkDataArray* positions)
{
  vtkNew<vtkPoints> points;
  points->SetData(positions);

  this->Mesh->SetPoints(points);
}

//------------------------------------------------------------------------------
void vtkF3DMemoryMesh::SetNormals(vtkDataArray* normals)
{
  this->Mesh->GetPointData()->SetNormals(normals);
}

//------------------------------------------------------------------------------
void vtkF3DMemoryMesh::SetTCoords(vtkDataArray* tcoords)
{
  this->Mesh->GetPointData()->SetTCoords(tcoords);
}

//------------------------------------------------------------------------------
void vtkF3DMemoryMesh::SetFaces(vtkDataArray* offsets, vtkDataArray* connectivity)
{
  vtkNew<vtkCellArray> polys;
  polys->SetData(offsets, connectivity);

  this->Mesh->SetPolys(polys);
}

//------------------------------------------------------------------------------
int vtkF3DMemoryMesh::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector)
{
  vtkPolyData* output = vtkPolyData::GetData(outputVector->GetInformationObject(0));

  output->ShallowCopy(this->Mesh);

  return 1;
}
