/**
 * @class   vtkF3DGaussianSplattingReader
 * @brief   VTK Reader for 3D Gaussian Splatting files
 *
 */

#ifndef vtkF3DGaussianSplattingReader_h
#define vtkF3DGaussianSplattingReader_h

#include <vtkPolyDataAlgorithm.h>

class vtkF3DGaussianSplattingReader : public vtkPolyDataAlgorithm
{
public:
  static vtkF3DGaussianSplattingReader* New();
  vtkTypeMacro(vtkF3DGaussianSplattingReader, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the file name.
   */
  vtkSetMacro(FileName, std::string);
  vtkGetMacro(FileName, std::string);
  ///@}

protected:
  vtkF3DGaussianSplattingReader();
  ~vtkF3DGaussianSplattingReader() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkF3DGaussianSplattingReader(const vtkF3DGaussianSplattingReader&) = delete;
  void operator=(const vtkF3DGaussianSplattingReader&) = delete;

  std::string FileName;
};

#endif
