#include "vtkF3DDepthSortPointCloud.h"

#include "vtkCamera.h"
#include "vtkCellData.h"
#include "vtkCharArray.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkFloatArray.h"
#include "vtkGenericCell.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIntArray.h"
#include "vtkLongArray.h"
#include "vtkLongLongArray.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProp3D.h"
#include "vtkShortArray.h"
#include "vtkSignedCharArray.h"
#include "vtkTransform.h"
#include "vtkUnsignedIntArray.h"
#include "vtkUnsignedLongArray.h"
#include "vtkUnsignedLongLongArray.h"
#include "vtkUnsignedShortArray.h"
#include "vtkSMPTools.h"
#include "vtkDataArrayRange.h"

#include <algorithm>
#include <numeric>

vtkStandardNewMacro(vtkF3DDepthSortPointCloud);

vtkCxxSetObjectMacro(vtkF3DDepthSortPointCloud, Camera, vtkCamera);

vtkF3DDepthSortPointCloud::vtkF3DDepthSortPointCloud()
  : Camera(nullptr)
{
  this->LastDirection[0] = this->LastDirection[1] = this->LastDirection[2] = 0.0;
}

vtkF3DDepthSortPointCloud::~vtkF3DDepthSortPointCloud()
{
  if (this->Camera)
  {
    this->Camera->Delete();
  }
}

void vtkF3DDepthSortPointCloud::ComputeProjectionVector(double direction[3])
{
  double* focalPoint = this->Camera->GetFocalPoint();
  double* origin = this->Camera->GetPosition();

  for (int i = 0; i < 3; ++i)
  {
    direction[i] = focalPoint[i] - origin[i];
  }

  vtkMath::Normalize(direction);
}

int vtkF3DDepthSortPointCloud::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPolyData* input = vtkPolyData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Compute the sort direction
  double direction[3] = { 0.0 };
  this->ComputeProjectionVector(direction);

  this->LastDirection[0] = direction[0];
  this->LastDirection[1] = direction[1];
  this->LastDirection[2] = direction[2];

  std::cout << "direction=" << direction[0] << " " << direction[1] << " " << direction[2] << std::endl;

  vtkIdType nbPoints = input->GetNumberOfPoints();

  if (this->Mapping->GetNumberOfTuples() != nbPoints)
  {
    this->Mapping->SetNumberOfTuples(nbPoints);
    auto rangeInit = vtk::DataArrayValueRange(this->Mapping);
    std::iota(rangeInit.begin(), rangeInit.end(), 0);
  }

  std::cout << "sorting start" << std::endl;

  this->LastDirection[0] = direction[0];
  this->LastDirection[1] = direction[1];
  this->LastDirection[2] = direction[2];

  std::vector<float> depths(nbPoints);

  vtkSMPTools::For(0, nbPoints, [&](vtkIdType first, vtkIdType last)
  {
    for (vtkIdType index = first; index < last; index++)
    {
      depths[index] = vtkMath::Dot(input->GetPoint(index), direction);
    }
  });

  auto rangeSort = vtk::DataArrayValueRange(this->Mapping);
  std::sort(rangeSort.begin(), rangeSort.end(), [&](vtkIdType left, vtkIdType right){
    return depths[left] > depths[right];
  });

  std::cout << "sorting end" << std::endl;

  this->LastPolyData->SetPoints(input->GetPoints());
  this->LastPolyData->GetPointData()->ShallowCopy(input->GetPointData());

  // create verts
  vtkNew<vtkCellArray> verts;
  verts->SetData(input->GetVerts()->GetOffsetsArray(), this->Mapping);
  this->LastPolyData->SetVerts(verts);

  output->ShallowCopy(this->LastPolyData);

  return 1;
}

vtkMTimeType vtkF3DDepthSortPointCloud::GetMTime()
{
  vtkMTimeType baseTime = this->Superclass::GetMTime();

  if (this->Camera && this->Camera->GetMTime() > baseTime)
  {
    // Compute the sort direction
    double direction[3] = { 0.0 };
    this->ComputeProjectionVector(direction);

    if (vtkMath::Dot(this->LastDirection, direction) > 0.99)
    {
      // same direction
      return baseTime;
    }

    return this->Camera->GetMTime();
  }

  return baseTime;
}

void vtkF3DDepthSortPointCloud::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->Camera)
  {
    os << indent << "Camera:\n";
    this->Camera->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << indent << "Camera: (none)\n";
  }
}
