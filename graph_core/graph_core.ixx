export module graph.core;

import <algorithm>;
import <any>;
import <cstdint>;
import <functional>;
import <optional>;
import <string>;
import <unordered_map>;
import <utility>;
import <vector>;

export namespace graph::core {
  struct NodeId {
    std::uint32_t value{};

    friend auto operator<=>(const NodeId&, const NodeId&) = default;
  };

  struct PortId {
    std::uint32_t value{};

    friend auto operator<=>(const PortId&, const PortId&) = default;
  };

  struct EdgeId {
    std::uint32_t value{};

    friend auto operator<=>(const EdgeId&, const EdgeId&) = default;
  };

  struct BusId {
    std::uint32_t value{};

    friend auto operator<=>(const BusId&, const BusId&) = default;
  };

  struct NodeIdHash {
    std::size_t operator()(const NodeId& id) const noexcept {
      return std::hash<std::uint32_t>{}(id.value);
    }
  };

  struct PortIdHash {
    std::size_t operator()(const PortId& id) const noexcept {
      return std::hash<std::uint32_t>{}(id.value);
    }
  };

  struct EdgeIdHash {
    std::size_t operator()(const EdgeId& id) const noexcept {
      return std::hash<std::uint32_t>{}(id.value);
    }
  };

  enum class PortDirection {
    Input,
    Output,
    Bidirectional
  };

  enum class PortMultiplicity {
    Single,
    Multiple
  };

  enum class ConnectError {
    None,
    UnknownPort,
    SamePort,
    DirectionMismatch,
    MultiplicityViolation
  };

  struct PortConstraints {
    PortDirection direction{PortDirection::Input};
    PortMultiplicity multiplicity{PortMultiplicity::Single};
    std::vector<std::string> allowed_kinds{};
  };

  struct Port {
    PortId id{};
    NodeId node{};
    std::string name{};
    PortConstraints constraints{};
    std::optional<BusId> bus{};
  };

  struct Node {
    NodeId id{};
    std::string name{};
    std::any payload{};
    std::vector<PortId> ports{};
  };

  struct Edge {
    EdgeId id{};
    PortId from{};
    PortId to{};
    std::optional<BusId> bus{};
  };

  struct ConnectResult {
    std::optional<EdgeId> edge{};
    ConnectError error{ConnectError::None};
  };

  struct GraphEvent {
    enum class Type {
      NodeAdded,
      NodeRemoved,
      PortAdded,
      PortRemoved,
      EdgeConnected,
      EdgeDisconnected
    };

    Type type{};
    std::optional<NodeId> node{};
    std::optional<PortId> port{};
    std::optional<EdgeId> edge{};
  };

  class Graph {
   public:
    NodeId add_node(std::string name, std::any payload = {}) {
      const NodeId id{next_node_id_++};
      Node node{.id = id, .name = std::move(name), .payload = std::move(payload)};
      nodes_.emplace(id, std::move(node));
      return id;
    }

    bool remove_node(NodeId id) {
      auto it = nodes_.find(id);
      if (it == nodes_.end()) {
        return false;
      }
      const auto ports = it->second.ports;
      for (const auto& port_id : ports) {
        remove_port(port_id);
      }
      nodes_.erase(it);
      return true;
    }

    PortId add_port(NodeId node_id, std::string name, PortConstraints constraints = {}) {
      const auto node_it = nodes_.find(node_id);
      if (node_it == nodes_.end()) {
        return {};
      }
      const PortId id{next_port_id_++};
      Port port{.id = id, .node = node_id, .name = std::move(name), .constraints = std::move(constraints)};
      ports_.emplace(id, port);
      node_it->second.ports.push_back(id);
      return id;
    }

    bool remove_port(PortId id) {
      auto port_it = ports_.find(id);
      if (port_it == ports_.end()) {
        return false;
      }
      const auto node_it = nodes_.find(port_it->second.node);
      if (node_it != nodes_.end()) {
        auto& port_list = node_it->second.ports;
        port_list.erase(std::remove(port_list.begin(), port_list.end(), id), port_list.end());
      }
      remove_edges_for_port(id);
      ports_.erase(port_it);
      return true;
    }

    ConnectResult connect_ports(PortId from, PortId to, std::optional<BusId> bus = {}) {
      if (from == to) {
        return {.edge = std::nullopt, .error = ConnectError::SamePort};
      }
      auto from_it = ports_.find(from);
      auto to_it = ports_.find(to);
      if (from_it == ports_.end() || to_it == ports_.end()) {
        return {.edge = std::nullopt, .error = ConnectError::UnknownPort};
      }
      if (!direction_allows_connection(from_it->second.constraints.direction, to_it->second.constraints.direction)) {
        return {.edge = std::nullopt, .error = ConnectError::DirectionMismatch};
      }
      if (!multiplicity_allows_connection(from_it->second.constraints.multiplicity, from) ||
          !multiplicity_allows_connection(to_it->second.constraints.multiplicity, to)) {
        return {.edge = std::nullopt, .error = ConnectError::MultiplicityViolation};
      }
      const EdgeId id{next_edge_id_++};
      Edge edge{.id = id, .from = from, .to = to, .bus = bus};
      edges_.emplace(id, edge);
      return {.edge = id, .error = ConnectError::None};
    }

    bool disconnect_edge(EdgeId id) {
      return edges_.erase(id) > 0;
    }

    const Node* find_node(NodeId id) const {
      if (auto it = nodes_.find(id); it != nodes_.end()) {
        return &it->second;
      }
      return nullptr;
    }

    const Port* find_port(PortId id) const {
      if (auto it = ports_.find(id); it != ports_.end()) {
        return &it->second;
      }
      return nullptr;
    }

    const Edge* find_edge(EdgeId id) const {
      if (auto it = edges_.find(id); it != edges_.end()) {
        return &it->second;
      }
      return nullptr;
    }

    std::vector<EdgeId> edges_for_port(PortId id) const {
      std::vector<EdgeId> results;
      for (const auto& [edge_id, edge] : edges_) {
        if (edge.from == id || edge.to == id) {
          results.push_back(edge_id);
        }
      }
      return results;
    }

   private:
    bool direction_allows_connection(PortDirection from, PortDirection to) const {
      if (from == PortDirection::Bidirectional || to == PortDirection::Bidirectional) {
        return true;
      }
      return from == PortDirection::Output && to == PortDirection::Input;
    }

    bool multiplicity_allows_connection(PortMultiplicity multiplicity, PortId id) const {
      if (multiplicity == PortMultiplicity::Multiple) {
        return true;
      }
      const auto edges = edges_for_port(id);
      return edges.empty();
    }

    void remove_edges_for_port(PortId id) {
      auto it = edges_.begin();
      while (it != edges_.end()) {
        if (it->second.from == id || it->second.to == id) {
          it = edges_.erase(it);
        } else {
          ++it;
        }
      }
    }

    std::uint32_t next_node_id_{1};
    std::uint32_t next_port_id_{1};
    std::uint32_t next_edge_id_{1};
    std::unordered_map<NodeId, Node, NodeIdHash> nodes_{};
    std::unordered_map<PortId, Port, PortIdHash> ports_{};
    std::unordered_map<EdgeId, Edge, EdgeIdHash> edges_{};
  };
} // namespace graph::core
