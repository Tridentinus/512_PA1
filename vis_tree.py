import re
import sys
import networkx as nx
import matplotlib.pyplot as plt


def parse_pa1_ttopo(filename):
    """
    Parse a PA1 .ttopo file (postorder traversal).
    Each line is either:
        <id>(<cap>)                -> leaf node
        (<left_len> <right_len> <k>) -> internal node with k parallel inverters
    
    In postorder: left child appears first, then right child, then parent.
    So on stack: right is on top, left is below.
    When we read (lenL lenR k), we pop RIGHT first, then LEFT.
    """
    with open(filename) as f:
        lines = [ln.strip() for ln in f if ln.strip()]

    stack = []
    nodes = {}
    edges = []
    node_id = 0

    for line in lines:
        # Leaf node: "2(3.5000000000e-14)"
        if not line.startswith("("):
            m_leaf = re.match(r"(\d+)\(([^)]+)\)", line)
            if m_leaf:
                label = int(m_leaf.group(1))
                cap = float(m_leaf.group(2))
                nid = f"Sink{label}"
                nodes[nid] = {
                    "type": "leaf",
                    "label": label,
                    "cap": cap,
                    "k": 0
                }
                stack.append(nid)
                continue
            else:
                print(f"⚠️  Cannot parse leaf line: {line}")
                continue

        # Internal node: "(left_len right_len k)"
        m_node = re.match(r"\(([^ ]+)\s+([^ ]+)(?:\s+(\d+))?\)", line)
        if not m_node:
            print(f"⚠️  Cannot parse internal node: {line}")
            continue

        left_len = float(m_node.group(1))
        right_len = float(m_node.group(2))
        k = int(m_node.group(3)) if m_node.group(3) else 0

        # In postorder: RIGHT child is on top of stack, LEFT child below
        right_child = stack.pop() if stack and right_len >= 0 else None
        left_child = stack.pop() if stack and left_len >= 0 else None

        node_id += 1
        nid = f"N{node_id}"
        
        # Determine node type
        if k > 0:
            node_type = "inverter"
        else:
            node_type = "internal"
        
        nodes[nid] = {
            "type": node_type,
            "k": k,
            "left_len": left_len,
            "right_len": right_len
        }

        # Add edges (parent -> child with length)
        if left_child and left_len >= 0:
            edges.append((nid, left_child, left_len))
        if right_child and right_len >= 0:
            edges.append((nid, right_child, right_len))

        stack.append(nid)

    root = stack[-1] if stack else None
    return nodes, edges, root


def draw_pa1_tree(nodes, edges, root, title=None):
    """Draw the tree with different colors for leaf/internal/inverter nodes."""
    G = nx.DiGraph()
    for src, dst, length in edges:
        G.add_edge(src, dst, length=length)

    # Use hierarchical layout
    try:
        pos = nx.nx_agraph.graphviz_layout(G, prog="dot")
    except:
        print("⚠️  Graphviz not available, using spring layout")
        pos = nx.spring_layout(G, seed=42, k=2)

    # Edge labels with "L:" and "R:" prefixes
    edge_labels = {}
    for src, dst, data in G.edges(data=True):
        length = data['length']
        # Determine if this is left or right child
        src_node = nodes[src]
        left_matches = any(e[0] == src and e[1] == dst and e[2] == src_node.get('left_len') 
                          for e in edges)
        prefix = "L:" if left_matches else "R:"
        edge_labels[(src, dst)] = f"{prefix}{length:.2e}"

    # Node colors and labels
    colors, labels = [], {}
    for n in G.nodes():
        node_data = nodes[n]
        node_type = node_data.get("type", "internal")
        
        if node_type == "leaf":
            label_num = node_data.get("label", "?")
            cap = node_data.get("cap", 0)
            labels[n] = f"Sink {label_num}\n{cap:.2e}F"
            colors.append("#90EE90")  # light green
        elif node_type == "inverter":
            k = node_data.get("k", 1)
            labels[n] = f"INV×{k}" if k > 1 else "INV"
            colors.append("#FFB347")  # orange
        else:  # internal
            labels[n] = n
            colors.append("#87CEEB")  # sky blue

    plt.figure(figsize=(18, 12))
    nx.draw(G, pos, labels=labels, node_color=colors,
            node_size=2500, arrows=True, font_size=9,
            edge_color='gray', width=2, arrowsize=20)
    nx.draw_networkx_edge_labels(G, pos, edge_labels=edge_labels, 
                                   font_size=7, font_color='darkred')
    
    plt.title(title or f"PA1 Tree (root={root})", fontsize=16, fontweight='bold')
    plt.axis('off')
    # plt.tight_layout()
    
    # Save and show
    output_file = "pa1_tree_visualization.png"
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"✅ Saved visualization to {output_file}")
    plt.show()


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python visualize_pa1.py <file.ttopo>")
        sys.exit(1)

    fname = sys.argv[1]
    nodes, edges, root = parse_pa1_ttopo(fname)
    
    print(f"\n✅ Parsed {len(nodes)} nodes, {len(edges)} edges")
    print(f"   Root: {root}")
    
    # Print some statistics
    leaf_count = sum(1 for n in nodes.values() if n.get("type") == "leaf")
    inv_count = sum(1 for n in nodes.values() if n.get("type") == "inverter")
    print(f"   Leaves: {leaf_count}, Inverters: {inv_count}")
    
    draw_pa1_tree(nodes, edges, root, title=f"Tree from {fname}")
