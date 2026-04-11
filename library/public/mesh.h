#ifndef f3d_mesh_h
#define f3d_mesh_h

#include "exception.h"
#include "export.h"

/// @cond
#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <vector>
/// @endcond

namespace f3d
{
/**
 * @class   mesh
 * @brief   Abstract class to represent a 3D surfacic mesh
 *
 * TODO
 */
class F3D_EXPORT mesh
{
public:

  /**
   * Get the temporal range
   */
  [[nodiscard]] virtual double getStartTime() const { return 0.0; }
  [[nodiscard]] virtual double getEndTime() const { return 0.0; }

  /**
   * Get the number of points in the mesh.
   */
  [[nodiscard]] virtual size_t getPointCount(double time) const = 0;

  /**
   * Get the position buffer of the mesh.
   */
  [[nodiscard]] virtual const float* getPoints(double time) const = 0;

  /**
   * Get the stride of the position buffer of the mesh.
   */
  [[nodiscard]] virtual size_t getPointsStride() const { return 3 * sizeof(float); }

  /**
   * Get the normal buffer of the mesh.
   */
  [[nodiscard]] virtual const float* getNormals(double time) const { return nullptr; }

  /**
   * Get the stride of the normal buffer of the mesh.
   */
  [[nodiscard]] virtual size_t getNormalsStride() const { return 3 * sizeof(float); }

  /**
   * Get the texture coordinates buffer of the mesh.
   */
  [[nodiscard]] virtual const float* getTextureCoordinates(double time) const { return nullptr; }

  /**
   * Get the stride of the texture coordinates buffer of the mesh.
   */
  [[nodiscard]] virtual size_t getTextureCoordinatesStride() const { return 2 * sizeof(float); }

  /**
   * Get the number of faces in the mesh.
   */
  [[nodiscard]] virtual size_t getFaceOffsetCount(double time) const = 0;

  /**
   * Get the face sides buffer of the mesh.
   */
  [[nodiscard]] virtual const unsigned int* getFaceOffsets(double time) const = 0;

  /**
   * Get the number of face indices in the mesh.
   */
  [[nodiscard]] virtual size_t getFaceIndexCount(double time) const = 0;

  /**
   * Get the face indices buffer of the mesh.
   */
  [[nodiscard]] virtual const unsigned int* getFaceIndices(double time) const = 0;

protected:
  //! @cond
  mesh() = default;
  virtual ~mesh() = default;
  mesh(const mesh&) = delete;
  mesh(mesh&&) = delete;
  mesh& operator=(const mesh&) = delete;
  mesh& operator=(mesh&&) = delete;
  //! @endcond
};
}

#endif
