#include "vtkF3DGaussianSplattingReader.h"

#include <vtkDataArrayRange.h>
#include <vtkDemandDrivenPipeline.h>
#include <vtkFloatArray.h>
#include <vtkInformation.h>
#include <vtkInformationVector.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>

#include "happly.h"

#include <numeric>

namespace
{
template<typename T>
vtkSmartPointer<vtkFloatArray> ConvertToFloatArray(const std::string& name,
  const std::vector<unsigned int> mapping, const std::vector<std::vector<float> >& components,
  T func)
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
      arr->SetTypedComponent(mapping[j], i, func(components[i][j]));
    }
  }

  return arr;
}

std::vector<unsigned int> ComputeMortonCodes(const std::array<std::vector<float>, 3>& components)
{
  size_t nbPts = components[0].size();
  std::vector<unsigned int> codes(nbPts);

  auto morton3D = [](float x, float y, float z) {
    auto expandBits = [](unsigned int v) {
      v = (v * 0x00010001u) & 0xFF0000FFu;
      v = (v * 0x00000101u) & 0x0F00F00Fu;
      v = (v * 0x00000011u) & 0xC30C30C3u;
      v = (v * 0x00000005u) & 0x49249249u;
      return v;
    };

    x = std::min(std::max(x * 1024.0f, 0.0f), 1023.0f);
    y = std::min(std::max(y * 1024.0f, 0.0f), 1023.0f);
    z = std::min(std::max(z * 1024.0f, 0.0f), 1023.0f);
    unsigned int xx = expandBits((unsigned int)x);
    unsigned int yy = expandBits((unsigned int)y);
    unsigned int zz = expandBits((unsigned int)z);
    return xx * 4 + yy * 2 + zz;
  };

  for (size_t j = 0; j < nbPts; j++)
  {
    codes[j] = morton3D(components[0][j], components[1][j], components[2][j]);
  }

  return codes;
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

  std::vector<unsigned int> codes = ComputeMortonCodes({ x, y, z });

  size_t nbPts = x.size();
  std::vector<unsigned int> mapping(nbPts);
  std::iota(mapping.begin(), mapping.end(), 0);

  std::sort(mapping.begin(), mapping.end(),
    [&](unsigned int left, unsigned int right) { return codes[left] < codes[right]; });

  auto id = [](float x) { return x; };

  auto sigmoid = [](float x) { return 1. / (1. + std::exp(-x)); };

  vtkNew<vtkPoints> points;
  points->SetDataTypeToFloat();
  points->SetData(ConvertToFloatArray("points", mapping, { x, y, z }, id));
  output->SetPoints(points);

  output->GetPointData()->SetScalars(
    ConvertToFloatArray("color", mapping, { f_dc_0, f_dc_1, f_dc_2 },
      [](float x) { return std::clamp(x * 0.282094791774 + 0.5, 0., 1.); }));
  output->GetPointData()->AddArray(ConvertToFloatArray("opacity", mapping, { opacity }, sigmoid));
  output->GetPointData()->AddArray(ConvertToFloatArray(
    "scale", mapping, { scale_0, scale_1, scale_2 }, [](float x) { return std::exp(x); }));
  output->GetPointData()->AddArray(
    ConvertToFloatArray("rotation", mapping, { rot_0, rot_1, rot_2, rot_3 }, id));

  // TODO: spherical harmonics

  // create verts
  vtkIdType nbPoints = points->GetNumberOfPoints();
  vtkNew<vtkIdTypeArray> offsets;
  vtkNew<vtkIdTypeArray> connectivity;
  offsets->SetNumberOfTuples(nbPoints + 1);
  connectivity->SetNumberOfTuples(nbPoints);

  auto offsetRange = vtk::DataArrayValueRange(offsets);
  std::iota(offsetRange.begin(), offsetRange.end(), 0);

  auto connectivityRange = vtk::DataArrayValueRange(connectivity);
  std::iota(connectivityRange.begin(), connectivityRange.end(), 0);

  vtkNew<vtkCellArray> verts;
  verts->SetData(offsets, connectivity);

  output->SetVerts(verts);

  return 1;
}

//----------------------------------------------------------------------------
void vtkF3DGaussianSplattingReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << this->FileName << "\n";
}
