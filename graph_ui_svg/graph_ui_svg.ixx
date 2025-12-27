export module graph.ui.svg;

import <cmath>;
import <iomanip>;
import <optional>;
import <sstream>;
import <string>;

import graph.layout;
import graph.ui;

export namespace graph::ui::svg {
  class SvgRenderer final : public graph::ui::Renderer {
   public:
    void begin_frame(double width, double height) override {
      width_ = width;
      height_ = height;
      stream_.str({});
      stream_.clear();
      stream_ << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"" << width
              << "\" height=\"" << height << "\" viewBox=\"0 0 " << width << " " << height
              << "\">\n";
    }

    void draw_scene(const graph::ui::Scene& scene) override {
      for (const auto& rect : scene.rectangles) {
        draw_rect(rect);
      }
      for (const auto& path : scene.paths) {
        draw_path(path);
      }
      for (const auto& label : scene.labels) {
        draw_text(label);
      }
    }

    void end_frame() override {
      stream_ << "</svg>\n";
    }

    [[nodiscard]] std::string str() const {
      return stream_.str();
    }

   private:
    void draw_rect(const graph::ui::RectShape& rect) {
      stream_ << "  <rect x=\"" << rect.rect.origin.x << "\" y=\"" << rect.rect.origin.y
              << "\" width=\"" << rect.rect.width << "\" height=\"" << rect.rect.height
              << "\"";
      if (rect.corner_radius > 0.0) {
        stream_ << " rx=\"" << rect.corner_radius << "\" ry=\"" << rect.corner_radius << "\"";
      }
      append_fill(rect.fill);
      append_stroke(rect.stroke);
      stream_ << " />\n";
    }

    void draw_text(const graph::ui::TextShape& text) {
      stream_ << "  <text x=\"" << text.position.x << "\" y=\"" << text.position.y << "\"";
      stream_ << " fill=\"" << color_to_css(text.style.color) << "\"";
      stream_ << " font-size=\"" << text.style.size << "\"";
      stream_ << " font-family=\"" << text.style.font << "\"";
      stream_ << ">" << escape_text(text.text) << "</text>\n";
    }

    void draw_path(const graph::ui::PathShape& path) {
      stream_ << "  <path d=\"" << path_to_d(path.path) << "\"";
      append_stroke(path.stroke);
      stream_ << " fill=\"none\"";
      stream_ << " />\n";
    }

    void append_fill(const std::optional<graph::ui::FillStyle>& fill) {
      if (fill) {
        stream_ << " fill=\"" << color_to_css(fill->color) << "\"";
      } else {
        stream_ << " fill=\"none\"";
      }
    }

    void append_stroke(const std::optional<graph::ui::StrokeStyle>& stroke) {
      if (!stroke) {
        stream_ << " stroke=\"none\"";
        return;
      }
      stream_ << " stroke=\"" << color_to_css(stroke->color) << "\"";
      stream_ << " stroke-width=\"" << stroke->width << "\"";
      if (stroke->dash_length && stroke->dash_gap) {
        stream_ << " stroke-dasharray=\"" << *stroke->dash_length << "," << *stroke->dash_gap
                << "\"";
      }
    }

    static std::string color_to_css(const graph::ui::Color& color) {
      std::ostringstream out;
      out << "rgba(" << static_cast<int>(color.r) << "," << static_cast<int>(color.g) << ","
          << static_cast<int>(color.b) << "," << std::fixed << std::setprecision(3)
          << (static_cast<double>(color.a) / 255.0) << ")";
      return out.str();
    }

    static std::string escape_text(const std::string& text) {
      std::string out;
      out.reserve(text.size());
      for (const auto ch : text) {
        switch (ch) {
          case '&':
            out += "&amp;";
            break;
          case '<':
            out += "&lt;";
            break;
          case '>':
            out += "&gt;";
            break;
          default:
            out += ch;
            break;
        }
      }
      return out;
    }

    static std::string path_to_d(const graph::layout::RoundedPath& path) {
      if (path.points.empty()) {
        return "";
      }

      std::ostringstream out;
      out << "M " << path.points.front().x << " " << path.points.front().y << " ";

      for (std::size_t i = 1; i < path.points.size(); ++i) {
        const auto& point = path.points[i];
        out << "L " << point.x << " " << point.y << " ";
      }

      return out.str();
    }

    double width_{0.0};
    double height_{0.0};
    std::ostringstream stream_{};
  };
}
