#include "vtkF3DGaussianSplattingReader.h"

#include <vtkDemandDrivenPipeline.h>
#include <vtkFloatArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>

#include "happly.h"

namespace
{
template<typename T>
vtkSmartPointer<vtkFloatArray> ConvertToFloatArray(const std::string& name, const std::vector<std::vector<float>>& components, T func)
{
  vtkNew<vtkFloatArray> arr;

  if (components.size() < 1)
  {
    return nullptr;
  }

  size_t nbPts = components[0].size();

  for (size_t i = 1; i < components.size(); i++)
  {
    if (components[i].size() != nbPts)
    {
      return nullptr;
    }
  }

  arr->SetNumberOfComponents(components.size());
  arr->SetNumberOfTuples(nbPts);
  arr->SetName(name.c_str());

  for (size_t i = 0; i < components.size(); i++)
  {
    for (size_t j = 0; j < nbPts; j++)
    {
      arr->SetTypedComponent(j, i, func(components[i][j]));
    }
  }

  return arr;
}
}

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkF3DGaussianSplattingReader);

//----------------------------------------------------------------------------
vtkF3DGaussianSplattingReader::vtkF3DGaussianSplattingReader()
{
  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
vtkF3DGaussianSplattingReader::~vtkF3DGaussianSplattingReader() = default;

//----------------------------------------------------------------------------
int vtkF3DGaussianSplattingReader::RequestData(
  vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector)
{
  vtkPolyData* output = vtkPolyData::GetData(outputVector);

  // Construct a data object by reading from file
  happly::PLYData plyIn(this->FileName);

  // Get data from the object
  std::vector<float> x = plyIn.getElement("vertex").getProperty<float>("x");
  std::vector<float> y = plyIn.getElement("vertex").getProperty<float>("y");
  std::vector<float> z = plyIn.getElement("vertex").getProperty<float>("z");
  std::vector<float> nx = plyIn.getElement("vertex").getProperty<float>("nx");
  std::vector<float> ny = plyIn.getElement("vertex").getProperty<float>("ny");
  std::vector<float> nz = plyIn.getElement("vertex").getProperty<float>("nz");
  std::vector<float> f_dc_0 = plyIn.getElement("vertex").getProperty<float>("f_dc_0");
  std::vector<float> f_dc_1 = plyIn.getElement("vertex").getProperty<float>("f_dc_1");
  std::vector<float> f_dc_2 = plyIn.getElement("vertex").getProperty<float>("f_dc_2");
  std::vector<float> opacity = plyIn.getElement("vertex").getProperty<float>("opacity");
  std::vector<float> scale_0 = plyIn.getElement("vertex").getProperty<float>("scale_0");
  std::vector<float> scale_1 = plyIn.getElement("vertex").getProperty<float>("scale_1");
  std::vector<float> scale_2 = plyIn.getElement("vertex").getProperty<float>("scale_2");
  std::vector<float> rot_0 = plyIn.getElement("vertex").getProperty<float>("rot_0");
  std::vector<float> rot_1 = plyIn.getElement("vertex").getProperty<float>("rot_1");
  std::vector<float> rot_2 = plyIn.getElement("vertex").getProperty<float>("rot_2");
  std::vector<float> rot_3 = plyIn.getElement("vertex").getProperty<float>("rot_3");

  std::vector<float> f_rest_0 = plyIn.getElement("vertex").getProperty<float>("f_rest_0");
  std::vector<float> f_rest_1 = plyIn.getElement("vertex").getProperty<float>("f_rest_1");
  std::vector<float> f_rest_2 = plyIn.getElement("vertex").getProperty<float>("f_rest_2");

  auto id = [](float v){ return v; };

  vtkNew<vtkPoints> points;
  points->SetDataTypeToFloat();
  points->SetData(ConvertToFloatArray("points", {x, y, z}, id));
  output->SetPoints(points);
  
  output->GetPointData()->SetNormals(ConvertToFloatArray("normals", {nx, ny, nz}, id));
  output->GetPointData()->SetScalars(ConvertToFloatArray("color", {f_dc_0, f_dc_1, f_dc_2}, [](float v){ return v*0.282094791774 + 0.5; }));
  output->GetPointData()->AddArray(ConvertToFloatArray("opacity", {opacity}, id));
  output->GetPointData()->AddArray(ConvertToFloatArray("scale", {scale_0, scale_1, scale_2}, id));

  // TODO: spherical harmonics


  return 1;
}

//----------------------------------------------------------------------------
void vtkF3DGaussianSplattingReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << this->FileName << "\n";
}
