#ifndef vtkF3DDepthSortPointCloud_h
#define vtkF3DDepthSortPointCloud_h

#include "vtkPolyDataAlgorithm.h"

class vtkCamera;

class vtkF3DDepthSortPointCloud : public vtkPolyDataAlgorithm
{
public:
  /**
   * Instantiate object.
   */
  static vtkF3DDepthSortPointCloud* New();

  vtkTypeMacro(vtkF3DDepthSortPointCloud, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify a camera that is used to define a view direction along which
   * the cells are sorted. This ivar only has effect if the direction is set
   * to front-to-back or back-to-front, and a camera is specified.
   */
  virtual void SetCamera(vtkCamera*);
  vtkGetObjectMacro(Camera, vtkCamera);
  ///@}

  /**
   * Return MTime also considering the dependent camera
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkF3DDepthSortPointCloud();
  ~vtkF3DDepthSortPointCloud() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  void ComputeProjectionVector(double direction[3]);

  vtkCamera* Camera;
  double LastDirection[3];
  vtkNew<vtkIdTypeArray> Mapping;
  vtkNew<vtkPolyData> LastPolyData;

private:
  vtkF3DDepthSortPointCloud(const vtkF3DDepthSortPointCloud&) = delete;
  void operator=(const vtkF3DDepthSortPointCloud&) = delete;
};

#endif
