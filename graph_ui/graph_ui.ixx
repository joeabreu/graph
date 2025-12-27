export module graph.ui;

import <cstdint>;
import <optional>;
import <string>;
import <utility>;
import <vector>;

import graph.layout;

export namespace graph::ui {
  struct Color {
    std::uint8_t r{255};
    std::uint8_t g{255};
    std::uint8_t b{255};
    std::uint8_t a{255};
  };

  struct StrokeStyle {
    Color color{};
    double width{1.0};
    std::optional<double> dash_length{};
    std::optional<double> dash_gap{};
  };

  struct FillStyle {
    Color color{};
  };

  struct TextStyle {
    Color color{};
    double size{12.0};
    std::string font{"sans-serif"};
  };

  struct RectShape {
    graph::layout::Rect rect{};
    double corner_radius{0.0};
    std::optional<FillStyle> fill{};
    std::optional<StrokeStyle> stroke{};
  };

  struct TextShape {
    graph::layout::Point position{};
    std::string text{};
    TextStyle style{};
  };

  struct PathShape {
    graph::layout::RoundedPath path{};
    StrokeStyle stroke{};
  };

  struct Scene {
    std::vector<RectShape> rectangles{};
    std::vector<TextShape> labels{};
    std::vector<PathShape> paths{};
  };

  class Renderer {
   public:
    virtual ~Renderer() = default;

    virtual void begin_frame(double width, double height) = 0;
    virtual void draw_scene(const Scene& scene) = 0;
    virtual void end_frame() = 0;
  };
}
