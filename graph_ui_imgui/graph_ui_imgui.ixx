export module graph.ui.imgui;

import <optional>;

import graph.layout;
import graph.ui;

export namespace graph::ui::imgui {
  class ImGuiRenderer final : public graph::ui::Renderer {
   public:
    void begin_frame(double width, double height) override {
      width_ = width;
      height_ = height;
    }

    void draw_scene(const graph::ui::Scene& scene) override {
      last_scene_ = &scene;
    }

    void end_frame() override {
      last_scene_ = nullptr;
    }

    [[nodiscard]] std::optional<graph::ui::Scene> snapshot() const {
      if (!last_scene_) {
        return std::nullopt;
      }
      return *last_scene_;
    }

   private:
    double width_{0.0};
    double height_{0.0};
    const graph::ui::Scene* last_scene_{nullptr};
  };
}
