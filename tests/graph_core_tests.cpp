#include <gtest/gtest.h>

import graph.core;

namespace graph::core::tests {

TEST(GraphCoreTests, AddAndFindNode) {
  Graph graph;
  const auto node_id = graph.add_node("Node A");
  const auto* node = graph.find_node(node_id);

  ASSERT_NE(node, nullptr);
  EXPECT_EQ(node->id, node_id);
  EXPECT_EQ(node->name, "Node A");
  EXPECT_TRUE(node->ports.empty());
}

TEST(GraphCoreTests, AddPortToNode) {
  Graph graph;
  const auto node_id = graph.add_node("Node A");
  PortConstraints constraints{.direction = PortDirection::Output};
  const auto port_id = graph.add_port(node_id, "Out", constraints);

  const auto* node = graph.find_node(node_id);
  const auto* port = graph.find_port(port_id);

  ASSERT_NE(node, nullptr);
  ASSERT_NE(port, nullptr);
  EXPECT_EQ(port->node, node_id);
  EXPECT_EQ(port->name, "Out");
  EXPECT_EQ(port->constraints.direction, PortDirection::Output);
  ASSERT_EQ(node->ports.size(), 1u);
  EXPECT_EQ(node->ports.front(), port_id);
}

TEST(GraphCoreTests, ConnectPortsHonorsDirection) {
  Graph graph;
  const auto src = graph.add_node("Source");
  const auto dst = graph.add_node("Target");
  PortConstraints out_constraints{.direction = PortDirection::Output};
  PortConstraints in_constraints{.direction = PortDirection::Input};

  const auto out_port = graph.add_port(src, "Out", out_constraints);
  const auto in_port = graph.add_port(dst, "In", in_constraints);

  const auto result = graph.connect_ports(out_port, in_port);
  EXPECT_EQ(result.error, ConnectError::None);
  ASSERT_TRUE(result.edge.has_value());

  const auto* edge = graph.find_edge(*result.edge);
  ASSERT_NE(edge, nullptr);
  EXPECT_EQ(edge->from, out_port);
  EXPECT_EQ(edge->to, in_port);
}

TEST(GraphCoreTests, ConnectPortsRejectsUnknownPorts) {
  Graph graph;
  const auto result = graph.connect_ports(PortId{1}, PortId{2});
  EXPECT_EQ(result.error, ConnectError::UnknownPort);
  EXPECT_FALSE(result.edge.has_value());
}

TEST(GraphCoreTests, ConnectPortsRejectsSamePort) {
  Graph graph;
  const auto node_id = graph.add_node("Node");
  const auto port_id = graph.add_port(node_id, "Port");

  const auto result = graph.connect_ports(port_id, port_id);
  EXPECT_EQ(result.error, ConnectError::SamePort);
  EXPECT_FALSE(result.edge.has_value());
}

TEST(GraphCoreTests, ConnectPortsRejectsDirectionMismatch) {
  Graph graph;
  const auto node_a = graph.add_node("A");
  const auto node_b = graph.add_node("B");
  PortConstraints out_constraints{.direction = PortDirection::Output};
  const auto out_a = graph.add_port(node_a, "OutA", out_constraints);
  const auto out_b = graph.add_port(node_b, "OutB", out_constraints);

  const auto result = graph.connect_ports(out_a, out_b);
  EXPECT_EQ(result.error, ConnectError::DirectionMismatch);
  EXPECT_FALSE(result.edge.has_value());
}

TEST(GraphCoreTests, ConnectPortsHonorsMultiplicity) {
  Graph graph;
  const auto node_a = graph.add_node("A");
  const auto node_b = graph.add_node("B");
  const auto node_c = graph.add_node("C");
  PortConstraints out_constraints{.direction = PortDirection::Output, .multiplicity = PortMultiplicity::Single};
  PortConstraints in_constraints{.direction = PortDirection::Input, .multiplicity = PortMultiplicity::Single};
  const auto out_port = graph.add_port(node_a, "Out", out_constraints);
  const auto in_b = graph.add_port(node_b, "InB", in_constraints);
  const auto in_c = graph.add_port(node_c, "InC", in_constraints);

  const auto first = graph.connect_ports(out_port, in_b);
  EXPECT_EQ(first.error, ConnectError::None);
  ASSERT_TRUE(first.edge.has_value());

  const auto second = graph.connect_ports(out_port, in_c);
  EXPECT_EQ(second.error, ConnectError::MultiplicityViolation);
  EXPECT_FALSE(second.edge.has_value());
}

TEST(GraphCoreTests, RemovePortCleansEdges) {
  Graph graph;
  const auto node_a = graph.add_node("A");
  const auto node_b = graph.add_node("B");
  PortConstraints out_constraints{.direction = PortDirection::Output};
  PortConstraints in_constraints{.direction = PortDirection::Input};
  const auto out_port = graph.add_port(node_a, "Out", out_constraints);
  const auto in_port = graph.add_port(node_b, "In", in_constraints);

  const auto connection = graph.connect_ports(out_port, in_port);
  ASSERT_TRUE(connection.edge.has_value());
  EXPECT_NE(graph.find_edge(*connection.edge), nullptr);

  EXPECT_TRUE(graph.remove_port(in_port));
  EXPECT_EQ(graph.find_edge(*connection.edge), nullptr);
  EXPECT_EQ(graph.find_port(in_port), nullptr);
}

TEST(GraphCoreTests, RemoveNodeRemovesPortsAndEdges) {
  Graph graph;
  const auto node_a = graph.add_node("A");
  const auto node_b = graph.add_node("B");
  PortConstraints out_constraints{.direction = PortDirection::Output};
  PortConstraints in_constraints{.direction = PortDirection::Input};
  const auto out_port = graph.add_port(node_a, "Out", out_constraints);
  const auto in_port = graph.add_port(node_b, "In", in_constraints);
  const auto connection = graph.connect_ports(out_port, in_port);

  ASSERT_TRUE(connection.edge.has_value());
  EXPECT_NE(graph.find_edge(*connection.edge), nullptr);

  EXPECT_TRUE(graph.remove_node(node_a));
  EXPECT_EQ(graph.find_node(node_a), nullptr);
  EXPECT_EQ(graph.find_port(out_port), nullptr);
  EXPECT_EQ(graph.find_edge(*connection.edge), nullptr);
}

TEST(GraphCoreTests, DisconnectEdgeRemovesConnection) {
  Graph graph;
  const auto node_a = graph.add_node("A");
  const auto node_b = graph.add_node("B");
  PortConstraints out_constraints{.direction = PortDirection::Output};
  PortConstraints in_constraints{.direction = PortDirection::Input};
  const auto out_port = graph.add_port(node_a, "Out", out_constraints);
  const auto in_port = graph.add_port(node_b, "In", in_constraints);
  const auto connection = graph.connect_ports(out_port, in_port);

  ASSERT_TRUE(connection.edge.has_value());
  EXPECT_TRUE(graph.disconnect_edge(*connection.edge));
  EXPECT_EQ(graph.find_edge(*connection.edge), nullptr);
}

TEST(GraphCoreTests, EdgesForPortReturnsAllConnections) {
  Graph graph;
  const auto node_a = graph.add_node("A");
  const auto node_b = graph.add_node("B");
  const auto node_c = graph.add_node("C");
  PortConstraints out_constraints{.direction = PortDirection::Output, .multiplicity = PortMultiplicity::Multiple};
  PortConstraints in_constraints{.direction = PortDirection::Input};
  const auto out_port = graph.add_port(node_a, "Out", out_constraints);
  const auto in_b = graph.add_port(node_b, "InB", in_constraints);
  const auto in_c = graph.add_port(node_c, "InC", in_constraints);

  const auto edge_b = graph.connect_ports(out_port, in_b);
  const auto edge_c = graph.connect_ports(out_port, in_c);

  ASSERT_TRUE(edge_b.edge.has_value());
  ASSERT_TRUE(edge_c.edge.has_value());

  const auto edges = graph.edges_for_port(out_port);
  EXPECT_EQ(edges.size(), 2u);
}

}  // namespace graph::core::tests
