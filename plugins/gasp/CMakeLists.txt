cmake_minimum_required(VERSION 3.21)

project(f3d-plugin-nerf)

include(GNUInstallDirs)

# Check if the plugin is built externally
if(PROJECT_IS_TOP_LEVEL)
  find_package(f3d REQUIRED)
else()
  include(f3dPlugin)
endif()

find_package(meshoptimizer REQUIRED)

f3d_plugin_init()

f3d_plugin_declare_reader(
  NAME 3d
  EXTENSIONS ply
  SCORE 90
  MIMETYPES application/vnd.ply
  VTK_READER vtkF3DGaussianSplattingReader
  FORMAT_DESCRIPTION "3D Gaussian Splatting"
)

f3d_plugin_build(
  NAME gasp
  VERSION 1.0
  DESCRIPTION "3D Gaussian Splatting"
)
