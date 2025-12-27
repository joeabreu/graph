export module graph.layout;

import <algorithm>;
import <cmath>;
import <cstddef>;
import <optional>;
import <span>;
import <utility>;
import <vector>;

export namespace graph::layout {
  struct Point {
    double x{};
    double y{};

    friend bool operator==(const Point&, const Point&) = default;
  };

  struct Rect {
    Point origin{};
    double width{};
    double height{};

    [[nodiscard]] Point center() const {
      return {origin.x + width * 0.5, origin.y + height * 0.5};
    }
  };

  enum class Direction {
    Up,
    Down,
    Left,
    Right
  };

  struct Anchor {
    Point position{};
    Direction normal{Direction::Right};
  };

  struct Segment {
    Point from{};
    Point to{};

    [[nodiscard]] bool is_horizontal() const {
      return from.y == to.y;
    }

    [[nodiscard]] bool is_vertical() const {
      return from.x == to.x;
    }
  };

  struct Polyline {
    std::vector<Point> points{};
  };

  struct BusGroup {
    std::vector<std::size_t> path_indices{};
  };

  struct BusLanes {
    Polyline centerline{};
    std::vector<Polyline> lanes{};
    std::vector<double> lane_offsets{};
  };

  struct RoundedPath {
    std::vector<Point> points{};
    std::vector<double> corner_radii{};
  };

  [[nodiscard]] inline Point add(Point left, Point right) {
    return {left.x + right.x, left.y + right.y};
  }

  [[nodiscard]] inline Point subtract(Point left, Point right) {
    return {left.x - right.x, left.y - right.y};
  }

  [[nodiscard]] inline Point scale(Point point, double factor) {
    return {point.x * factor, point.y * factor};
  }

  [[nodiscard]] inline double length(Point point) {
    return std::sqrt(point.x * point.x + point.y * point.y);
  }

  [[nodiscard]] inline Point normalize(Point point) {
    const auto len = length(point);
    if (len == 0.0) {
      return {};
    }
    return {point.x / len, point.y / len};
  }

  [[nodiscard]] inline Point move_point(Point point, Direction dir, double distance) {
    switch (dir) {
      case Direction::Up:
        point.y -= distance;
        break;
      case Direction::Down:
        point.y += distance;
        break;
      case Direction::Left:
        point.x -= distance;
        break;
      case Direction::Right:
        point.x += distance;
        break;
    }
    return point;
  }

  [[nodiscard]] inline Direction segment_direction(const Point& from, const Point& to) {
    if (from.x == to.x) {
      return (to.y >= from.y) ? Direction::Down : Direction::Up;
    }
    return (to.x >= from.x) ? Direction::Right : Direction::Left;
  }

  [[nodiscard]] inline Point left_normal(Direction dir) {
    switch (dir) {
      case Direction::Up:
        return {-1.0, 0.0};
      case Direction::Down:
        return {1.0, 0.0};
      case Direction::Left:
        return {0.0, 1.0};
      case Direction::Right:
        return {0.0, -1.0};
    }
    return {};
  }

  [[nodiscard]] inline Polyline simplify_collinear(Polyline polyline) {
    if (polyline.points.size() < 3) {
      return polyline;
    }

    std::vector<Point> simplified;
    simplified.reserve(polyline.points.size());
    simplified.push_back(polyline.points.front());

    for (std::size_t i = 1; i + 1 < polyline.points.size(); ++i) {
      const auto& prev = simplified.back();
      const auto& current = polyline.points[i];
      const auto& next = polyline.points[i + 1];

      const bool collinear = (prev.x == current.x && current.x == next.x) ||
                             (prev.y == current.y && current.y == next.y);
      if (!collinear) {
        simplified.push_back(current);
      }
    }

    simplified.push_back(polyline.points.back());
    polyline.points = std::move(simplified);
    return polyline;
  }

  [[nodiscard]] inline Polyline route_orthogonal(const Anchor& start,
                                                const Anchor& end,
                                                double lead_out_distance) {
    const auto start_lead = move_point(start.position, start.normal, lead_out_distance);
    const auto end_lead = move_point(end.position, end.normal, lead_out_distance);

    Polyline polyline;
    polyline.points.push_back(start.position);
    polyline.points.push_back(start_lead);

    if (start_lead.x != end_lead.x && start_lead.y != end_lead.y) {
      const Point mid{end_lead.x, start_lead.y};
      polyline.points.push_back(mid);
    }

    polyline.points.push_back(end_lead);
    polyline.points.push_back(end.position);

    return simplify_collinear(std::move(polyline));
  }

  [[nodiscard]] inline Polyline offset_polyline(const Polyline& polyline, double offset) {
    if (polyline.points.size() < 2 || offset == 0.0) {
      return polyline;
    }

    Polyline result;
    result.points.resize(polyline.points.size());

    for (std::size_t i = 0; i < polyline.points.size(); ++i) {
      std::optional<Direction> prev_dir;
      std::optional<Direction> next_dir;

      if (i > 0) {
        prev_dir = segment_direction(polyline.points[i - 1], polyline.points[i]);
      }
      if (i + 1 < polyline.points.size()) {
        next_dir = segment_direction(polyline.points[i], polyline.points[i + 1]);
      }

      Point normal{};
      double scale_factor = offset;

      if (prev_dir && next_dir) {
        const auto prev_normal = left_normal(*prev_dir);
        const auto next_normal = left_normal(*next_dir);
        if (prev_normal == next_normal) {
          normal = prev_normal;
        } else {
          normal = normalize(add(prev_normal, next_normal));
          scale_factor = offset * std::sqrt(2.0);
        }
      } else if (prev_dir) {
        normal = left_normal(*prev_dir);
      } else if (next_dir) {
        normal = left_normal(*next_dir);
      }

      result.points[i] = add(polyline.points[i], scale(normal, scale_factor));
    }

    return result;
  }

  [[nodiscard]] inline BusLanes build_bus_lanes(const Polyline& centerline,
                                               std::size_t lane_count,
                                               double lane_spacing) {
    BusLanes bus;
    bus.centerline = centerline;
    bus.lanes.reserve(lane_count);
    bus.lane_offsets.reserve(lane_count);

    if (lane_count == 0) {
      return bus;
    }

    const double center_index = static_cast<double>(lane_count - 1) * 0.5;
    for (std::size_t i = 0; i < lane_count; ++i) {
      const double offset = (static_cast<double>(i) - center_index) * lane_spacing;
      bus.lane_offsets.push_back(offset);
      bus.lanes.push_back(offset_polyline(centerline, offset));
    }

    return bus;
  }

  [[nodiscard]] inline std::vector<BusGroup> auto_group_parallel_paths(std::span<const Polyline> paths,
                                                                       double proximity_threshold) {
    std::vector<BusGroup> groups;

    for (std::size_t i = 0; i < paths.size(); ++i) {
      const auto& path = paths[i];
      if (path.points.size() < 2) {
        continue;
      }
      const auto dir = segment_direction(path.points[0], path.points[1]);
      const auto midpoint = scale(add(path.points[0], path.points[1]), 0.5);

      bool grouped = false;
      for (auto& group : groups) {
        const auto& candidate = paths[group.path_indices.front()];
        if (candidate.points.size() < 2) {
          continue;
        }
        const auto candidate_dir = segment_direction(candidate.points[0], candidate.points[1]);
        if (candidate_dir != dir) {
          continue;
        }
        const auto candidate_midpoint = scale(add(candidate.points[0], candidate.points[1]), 0.5);
        const auto delta = subtract(midpoint, candidate_midpoint);
        if (length(delta) <= proximity_threshold) {
          group.path_indices.push_back(i);
          grouped = true;
          break;
        }
      }

      if (!grouped) {
        groups.push_back(BusGroup{.path_indices = {i}});
      }
    }

    return groups;
  }

  [[nodiscard]] inline RoundedPath apply_corner_radius(const Polyline& polyline, double radius) {
    RoundedPath rounded;
    rounded.points = polyline.points;
    rounded.corner_radii.resize(polyline.points.size(), 0.0);

    if (polyline.points.size() < 3) {
      return rounded;
    }

    for (std::size_t i = 1; i + 1 < polyline.points.size(); ++i) {
      const auto& prev = polyline.points[i - 1];
      const auto& current = polyline.points[i];
      const auto& next = polyline.points[i + 1];
      const bool turn = (prev.x == current.x && current.y == next.y) ||
                        (prev.y == current.y && current.x == next.x);
      if (turn) {
        rounded.corner_radii[i] = radius;
      }
    }

    return rounded;
  }
}
