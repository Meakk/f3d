#include "PseudoUnitTest.h"
#include "TestSDKHelpers.h"

#include <engine.h>
#include <interactor.h>
#include <log.h>
#include <mesh.h>
#include <scene.h>
#include <window.h>

struct WavyGridMesh
{
  struct Vertex
  {
    float position[3] = { 0.0, 0.0, 0.0 };
    float normal[3] = { 0.0, 0.0, 1.0 };
    float uv[2] = { 0.0, 0.0 };
  };

  struct Quad
  {
    unsigned int i0, i1, i2, i3;
  };

  WavyGridMesh(int nx, int ny)
  {
    // Initialize flat grid
    constexpr float size = 1.0f;

    this->Vertices.resize((nx + 1) * (ny + 1));

    for (int j = 0; j <= ny; ++j)
    {
      for (int i = 0; i <= nx; ++i)
      {
        float u = static_cast<float>(i) / nx;
        float v = static_cast<float>(j) / ny;

        Vertex& vert = this->Vertices[j * (nx + 1) + i];

        vert.position[0] = (u - 0.5f) * size * 2.0f;
        vert.position[1] = (v - 0.5f) * size * 2.0f;
        vert.position[2] = 0.f;
        vert.normal[0] = 0.f;
        vert.normal[1] = 0.f;
        vert.normal[2] = 1.f;
        vert.uv[0] = u;
        vert.uv[1] = v;
      }
    }

    // Initialize quads
    this->Quads.reserve(nx * ny);
    for (int j = 0; j < ny; ++j)
    {
      for (int i = 0; i < nx; ++i)
      {
        unsigned int i0 = j * (nx + 1) + i;
        unsigned int i1 = j * (nx + 1) + (i + 1);
        unsigned int i2 = (j + 1) * (nx + 1) + (i + 1);
        unsigned int i3 = (j + 1) * (nx + 1) + i;

        this->Quads.push_back({ i0, i1, i2, i3 });
      }
    }

    this->Update(0.0f);
  }

  void Update(float time)
  {
    // Update height field
    constexpr float amplitude = 0.1f;
    constexpr float freq = 4.0f;

    for (Vertex& vert : this->Vertices)
    {
        // Height field
        vert.position[2] = amplitude *
                  std::sin(freq * vert.position[0] + time) *
                  std::cos(freq * vert.position[1] + time);

        // Analytical normal computation
        float dzdx = amplitude * freq *
                    std::cos(freq * vert.position[0] + time) *
                    std::cos(freq * vert.position[1] + time);

        float dzdy = amplitude * freq *
                    std::sin(freq * vert.position[0] + time) *
                    -std::sin(freq * vert.position[1] + time);

        // Normal = normalize(-dz/dx, -dz/dy, 1)
        const float nxv = -dzdx;
        const float nyv = -dzdy;
        constexpr float nzv = 1.0f;

        float len = std::sqrt(nxv * nxv + nyv * nyv + nzv * nzv);

        vert.normal[0] = nxv / len;
        vert.normal[1] = nyv / len;
        vert.normal[2] = nzv / len;
    }
  }

  std::vector<Vertex> Vertices;
  std::vector<Quad> Quads;
};

class ZeroCopyMesh : public f3d::mesh
{
public:
  ZeroCopyMesh(const WavyGridMesh& grid): Grid(grid)
  {
    this->FaceOffsets.resize(this->Grid.Quads.size() + 1);
    std::generate(this->FaceOffsets.begin(), this->FaceOffsets.end(), [n = 0]() mutable {
      const unsigned int offset = n;
      n += 4;
      return offset;
    });
  }

  size_t getPointCount(double time) const override
  {
    return this->Grid.Vertices.size();
  }

  const float* getPoints(double time) const override
  {
    return reinterpret_cast<const float*>(this->Grid.Vertices.data());
  }

  size_t getPointsStride() const override
  {
    return 8;
  }

  const float* getNormals(double time) const override
  {
    return reinterpret_cast<const float*>(this->Grid.Vertices.data()) + 3;
  }

  size_t getNormalsStride() const override
  {
    return 8;
  }

  const float* getTextureCoordinates(double time) const override
  {
    return reinterpret_cast<const float*>(this->Grid.Vertices.data()) + 6;
  }

  size_t getTextureCoordinatesStride() const override
  {
    return 8;
  }

  size_t getFaceOffsetCount(double time) const override
  {
    return this->FaceOffsets.size();
  }

  const unsigned int* getFaceOffsets(double time) const override
  {
    return this->FaceOffsets.data();
  }

  size_t getFaceIndexCount(double time) const override
  {
    return this->Grid.Quads.size() * 4;
  }

  const unsigned int* getFaceIndices(double time) const override
  {
    return reinterpret_cast<const unsigned int*>(this->Grid.Quads.data());
  }

private:
  const WavyGridMesh& Grid;
  std::vector<unsigned int> FaceOffsets;
};

int TestSDKSceneFromMemoryZeroCopy([[maybe_unused]] int argc, [[maybe_unused]] char* argv[])
{
  PseudoUnitTest test;

  f3d::log::setVerboseLevel(f3d::log::VerboseLevel::DEBUG);
  f3d::engine eng = f3d::engine::create();
  f3d::scene& sce = eng.getScene();
  f3d::interactor& inter = eng.getInteractor();
  f3d::window& win = eng.getWindow().setSize(300, 300);

  // 

  std::string texturePath = std::string(argv[1]) + "data/world.png";
  eng.getOptions().model.color.texture = texturePath;
  eng.getOptions().ui.animation_progress = true;

  WavyGridMesh grid(20, 20);

  ZeroCopyMesh mesh(grid);

  // Add mesh from memory (zero-copy)
  test("add mesh from memory", [&]() {
    sce.add(mesh);
  });

  inter.start();

  // Advance loop
  //inter.startAnimation();
  //inter.triggerEventLoop(1.0);

  // Render test
  test("render mesh at time 1.0",
    TestSDKHelpers::RenderTest(
      win, std::string(argv[1]) + "baselines/", argv[2], "TestSDKSceneFromMemoryZeroCopy1"));

  inter.triggerEventLoop(1.0);

  // Render test
  test("render mesh at time 2.0",
    TestSDKHelpers::RenderTest(
      win, std::string(argv[1]) + "baselines/", argv[2], "TestSDKSceneFromMemoryZeroCopy2"));

  return test.result();
}
