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

  if (this->Mapping.size() != nbPoints)
  {
    this->Mapping.resize(nbPoints);
    std::iota(this->Mapping.begin(), this->Mapping.end(), 0);
  }

  std::cout << "sorting start" << std::endl;

  this->LastDirection[0] = direction[0];
  this->LastDirection[1] = direction[1];
  this->LastDirection[2] = direction[2];

  std::sort(this->Mapping.begin(), this->Mapping.end(), [&](vtkIdType left, vtkIdType right){
    return vtkMath::Dot(input->GetPoint(left), direction) > vtkMath::Dot(input->GetPoint(right), direction);
  });

  std::cout << "sorting end" << std::endl;

  vtkNew<vtkFloatArray> arr;
  arr->SetNumberOfComponents(3);
  arr->SetNumberOfTuples(nbPoints);

  for (vtkIdType i = 0; i < nbPoints; i++)
  {
    arr->SetTuple(i, this->Mapping[i], input->GetPoints()->GetData());
  }

  vtkNew<vtkPoints> points;
  points->SetDataTypeToFloat();
  points->SetData(arr);

  this->LastPolyData->SetPoints(points);

  vtkPointData* sourcePointData = input->GetPointData();
  vtkPointData* destPointData = this->LastPolyData->GetPointData();

  for (vtkIdType i = 0; i < sourcePointData->GetNumberOfArrays(); i++)
  {
    vtkDataArray* sourceArray = sourcePointData->GetArray(i);
    vtkDataArray* destArray = sourceArray->NewInstance();
    destArray->SetNumberOfComponents(sourceArray->GetNumberOfComponents());
    destArray->SetNumberOfTuples(sourceArray->GetNumberOfTuples());
    destArray->SetName(sourceArray->GetName());

    for (vtkIdType j = 0; j < nbPoints; j++)
    {
      destArray->SetTuple(j, this->Mapping[j], sourceArray);
    }

    destPointData->AddArray(destArray);
    destArray->Delete();
  }

  destPointData->SetActiveScalars(sourcePointData->GetScalars()->GetName());

  std::cout << "finished" << std::endl;

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