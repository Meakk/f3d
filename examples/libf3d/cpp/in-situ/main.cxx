#include <f3d/engine.h>
#include <f3d/interactor.h>
#include <f3d/options.h>
#include <f3d/scene.h>

#include <iostream>
#include <string>
#include <vector>

namespace
{
void AddCustomCommands(f3d::engine& eng)
{
  // Get the interactor
  f3d::interactor& inter = eng.getInteractor();
  inter.initCommands();

  // Reset options to initial state
  f3d::options& opt = eng.getOptions();
  // Keep a copy of the initial options
  const f3d::options initialOptions = opt;
  inter.addCommand(
    "reset_options",
    [&eng, initialOptions](const std::vector<std::string>&) { eng.getOptions() = initialOptions; },
    f3d::interactor::command_documentation_t{ "reset_options", "Reset basic render/UI options" });

  // Increase animation speed factor
  inter.addCommand(
    "increase_animation_speed_factor",
    [&eng](const std::vector<std::string>&)
    {
      auto& o = eng.getOptions();
      o.scene.animation.speed_factor = f3d::ratio_t(o.scene.animation.speed_factor + 0.05);
    },
    f3d::interactor::command_documentation_t{
      "increase_animation_speed_factor", "Increase animation speed factor" });

  // Decrease animation speed factor
  inter.addCommand(
    "decrease_animation_speed_factor",
    [&eng](const std::vector<std::string>&)
    {
      auto& o = eng.getOptions();
      o.scene.animation.speed_factor = f3d::ratio_t(o.scene.animation.speed_factor - 0.05);
    },
    f3d::interactor::command_documentation_t{
      "decrease_animation_speed_factor", "Decrease animation speed factor" });

  // Toggle grid visibility
  inter.removeCommand("toggle_grid");
  inter.addCommand(
    "toggle_grid",
    [&eng](const std::vector<std::string>&)
    {
      auto& o = eng.getOptions();
      o.render.grid.enable = !o.render.grid.enable;
    },
    f3d::interactor::command_documentation_t{ "toggle_grid", "Toggle ground grid visibility" });

  // Toggle axis
  inter.removeCommand("toggle_axis");
  inter.addCommand(
    "toggle_axis",
    [&eng](const std::vector<std::string>&)
    {
      auto& o = eng.getOptions();
      o.ui.axis = !o.ui.axis;
    },
    f3d::interactor::command_documentation_t{ "toggle_axis", "Toggle axis" });

  // Toggle FXAA anti-aliasing
  inter.removeCommand("toggle_fxaa");
  inter.addCommand(
    "toggle_fxaa",
    [&eng](const std::vector<std::string>&)
    {
      auto& o = eng.getOptions();
      o.render.effect.antialiasing.enable = !o.render.effect.antialiasing.enable;
      // keep mode at fxaa in this example
      o.render.effect.antialiasing.mode = "fxaa";
    },
    f3d::interactor::command_documentation_t{ "toggle_fxaa", "Toggle FXAA anti-aliasing" });

  // Toggle tone mapping
  inter.removeCommand("toggle_tonemapping");
  inter.addCommand(
    "toggle_tonemapping",
    [&eng](const std::vector<std::string>&)
    {
      auto& o = eng.getOptions();
      o.render.effect.tone_mapping = !o.render.effect.tone_mapping;
    },
    f3d::interactor::command_documentation_t{ "toggle_tonemapping", "Toggle tone mapping" });
}

void AddCustomBindins(f3d::engine& eng)
{
  // Get the interactor
  f3d::interactor& inter = eng.getInteractor();
  inter.initBindings();

  f3d::options& opt = eng.getOptions();

  auto docTgl = [](const std::string& doc, const bool& val)
  { return std::pair(doc, (val ? "ON" : "OFF")); };
  auto docStr = [](const std::string& doc) { return std::pair(doc, ""); };
  auto docDblOpt = [](const std::string& doc, const std::optional<double>& val)
  {
    std::stringstream valStream;
    valStream.precision(2);
    valStream << std::fixed;
    if (val.has_value())
    {
      valStream << val.value();
    }
    else
    {
      valStream << "Unset";
    }
    return std::pair(doc, valStream.str());
  };

  // F: toggle FXAA
  inter.removeBinding({ f3d::interaction_bind_t::ModifierKeys::NONE, "F" });
  inter.addBinding(f3d::interaction_bind_t::parse("F"), "toggle render.effect.antialiasing.enable", "Rendering",
    std::bind(docTgl, "Toggle FXAA", std::cref(opt.render.effect.antialiasing.enable)),
    f3d::interactor::BindingType::TOGGLE);
}
}

int main(int argc, char** argv)
{
  // Load static/native plugins
  f3d::engine::autoloadPlugins();

  f3d::log::setVerboseLevel(f3d::log::VerboseLevel::DEBUG);

  // Create a native-window engine
  f3d::engine eng = f3d::engine::create();

  // Modify options using the struct API
  f3d::options& opt = eng.getOptions();
  opt.render.grid.enable = true;
  opt.render.grid.absolute = true;
  opt.render.show_edges = true;

  // UI overlays: axis + some HUD
  opt.ui.axis = true;
  opt.ui.fps = true;
  opt.ui.animation_progress = true;
  opt.ui.filename = true;

  // FXAA + tone mapping
  opt.render.effect.antialiasing.enable = true;
  opt.render.effect.antialiasing.mode = "fxaa";
  opt.render.effect.tone_mapping = true;
  opt.scene.up_direction = { 0.0, 0.0, 1.0 };

  ::AddCustomCommands(eng);
  ::AddCustomBindins(eng);


  // Create vertex and index buffers for an uniform grid of quads
  f3d::mesh_t gridMesh;

  constexpr int gridSize = 100;
  for (int i = 0; i <= gridSize; i++)
  {
    for (int j = 0; j <= gridSize; j++)
    {
      gridMesh.points.push_back(-0.5f * gridSize + i);
      gridMesh.points.push_back(-0.5f * gridSize + j);
      gridMesh.points.push_back(100.0f);
      gridMesh.normals.push_back(0.0f);
      gridMesh.normals.push_back(0.0f);
      gridMesh.normals.push_back(1.0f);
      gridMesh.texture_coordinates.push_back(static_cast<float>(i) / gridSize);
      gridMesh.texture_coordinates.push_back(static_cast<float>(j) / gridSize);
    }
  }

  for (int i = 0; i < gridSize; i++)
  {
    for (int j = 0; j < gridSize; j++)
    {
      unsigned int topLeft = (i + 1) * (gridSize + 1) + j;
      unsigned int topRight = topLeft + 1;
      unsigned int bottomLeft = i * (gridSize + 1) + j;
      unsigned int bottomRight = bottomLeft + 1;
      gridMesh.face_sides.push_back(4);
      gridMesh.face_indices.push_back(topRight);
      gridMesh.face_indices.push_back(bottomRight);
      gridMesh.face_indices.push_back(bottomLeft);
      gridMesh.face_indices.push_back(topLeft);
    }
  }

  try
  {
    // Add a model to the scene
    eng.getScene().add(gridMesh);
  }
  catch (const std::exception& e)
  {
    std::cerr << e.what() << '\n';
  }

  // Initial render
  f3d::window& win = eng.getWindow();
  win.setWindowName("libf3d in-situ example");
  win.setSize(1500, 1000);
  win.setPosition(500, 500);
  win.render();

  win.getCamera().setPosition({ -300.0f, -300.0f, 300.0f });
  win.getCamera().setFocalPoint({ 0.0f, 0.0f, 50.0f });
  win.getCamera().setViewUp({ 0.0f, 0.0f, 1.0f });

  // Start interaction loop
  f3d::interactor& inter = eng.getInteractor();
  if (argc > 2)
  {
    // For testing purposes only, shutdown the example after 1 second
    try
    {
      inter.start(1.0, [&inter]() { inter.stop(); });
    }
    catch (const std::exception& e)
    {
      std::cout << e.what() << std::endl;
      return EXIT_FAILURE;
    }
  }
  else
  {
    inter.start(0.0333333, []() {

      // Advance cloth simulation


    }); // ~30 FPS
  }

  return EXIT_SUCCESS;
}
