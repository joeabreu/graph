export module graph.editor;

import <optional>;
import <unordered_set>;
import <utility>;
import <vector>;

import graph.core;
import graph.layout;

export namespace graph::editor {
  struct PointerEvent {
    enum class Type {
      Down,
      Move,
      Up
    };

    Type type{};
    graph::layout::Point position{};
    bool primary_button{false};
    bool shift{false};
    bool ctrl{false};
  };

  struct KeyEvent {
    enum class Type {
      Down,
      Up
    };

    Type type{};
    int key{};
    bool shift{false};
    bool ctrl{false};
  };

  struct HitResult {
    std::optional<graph::core::NodeId> node{};
    std::optional<graph::core::PortId> port{};
  };

  struct SelectionState {
    std::unordered_set<graph::core::NodeId, graph::core::NodeIdHash> nodes{};
    std::unordered_set<graph::core::EdgeId, graph::core::EdgeIdHash> edges{};
  };

  struct DragState {
    bool active{false};
    graph::layout::Point start{};
    graph::layout::Point current{};
  };

  struct WirePreview {
    bool active{false};
    graph::core::PortId from{};
    graph::layout::Polyline path{};
  };

  struct EditorView {
    SelectionState selection{};
    std::optional<DragState> drag{};
    std::optional<WirePreview> preview{};
  };

  class HitTester {
   public:
    virtual ~HitTester() = default;
    virtual HitResult hit_test(const graph::layout::Point& position) const = 0;
  };

  class EditorPresenter {
   public:
    explicit EditorPresenter(graph::core::Graph& graph) : graph_(graph) {}

    void set_hit_tester(const HitTester* hit_tester) {
      hit_tester_ = hit_tester;
    }

    const EditorView& view() const {
      return view_;
    }

    void handle_pointer_event(const PointerEvent& event) {
      switch (event.type) {
        case PointerEvent::Type::Down:
          on_pointer_down(event);
          break;
        case PointerEvent::Type::Move:
          on_pointer_move(event);
          break;
        case PointerEvent::Type::Up:
          on_pointer_up(event);
          break;
      }
    }

    void handle_key_event(const KeyEvent&) {
      // Placeholder for future shortcuts (undo/redo, delete, etc.).
    }

   private:
    void on_pointer_down(const PointerEvent& event) {
      if (!event.primary_button) {
        return;
      }

      const auto hit = hit_tester_ ? hit_tester_->hit_test(event.position) : HitResult{};
      if (hit.port) {
        start_wire_preview(*hit.port, event.position);
        return;
      }

      if (hit.node) {
        begin_drag(event.position, event.shift, *hit.node);
        return;
      }

      clear_selection();
      view_.drag = DragState{.active = true, .start = event.position, .current = event.position};
    }

    void on_pointer_move(const PointerEvent& event) {
      if (view_.preview && view_.preview->active) {
        update_wire_preview(event.position);
        return;
      }
      if (view_.drag && view_.drag->active) {
        view_.drag->current = event.position;
      }
    }

    void on_pointer_up(const PointerEvent& event) {
      if (view_.preview && view_.preview->active) {
        finish_wire_preview(event.position);
        return;
      }
      if (view_.drag && view_.drag->active) {
        view_.drag.reset();
      }
    }

    void begin_drag(const graph::layout::Point& position, bool additive, graph::core::NodeId node) {
      if (!additive) {
        clear_selection();
      }
      view_.selection.nodes.insert(node);
      view_.drag = DragState{.active = true, .start = position, .current = position};
    }

    void clear_selection() {
      view_.selection.nodes.clear();
      view_.selection.edges.clear();
    }

    void start_wire_preview(graph::core::PortId port, const graph::layout::Point& position) {
      view_.preview = WirePreview{.active = true, .from = port};
      update_wire_preview(position);
    }

    void update_wire_preview(const graph::layout::Point& position) {
      if (!view_.preview) {
        return;
      }
      const graph::layout::Anchor start{.position = position, .normal = graph::layout::Direction::Right};
      const graph::layout::Anchor end{.position = position, .normal = graph::layout::Direction::Left};
      view_.preview->path = graph::layout::route_orthogonal(start, end, 12.0);
    }

    void finish_wire_preview(const graph::layout::Point& position) {
      if (!view_.preview) {
        return;
      }

      const auto hit = hit_tester_ ? hit_tester_->hit_test(position) : HitResult{};
      if (hit.port) {
        graph_.connect_ports(view_.preview->from, *hit.port);
      }
      view_.preview.reset();
    }

    graph::core::Graph& graph_;
    const HitTester* hit_tester_{nullptr};
    EditorView view_{};
  };
}
