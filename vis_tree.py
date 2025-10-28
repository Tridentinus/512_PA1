import re
import sys
import networkx as nx
import matplotlib.pyplot as plt


def parse_pa1_btopo(filename):
    """
    Parse a PA1 .btopo or .ttopo file (postorder traversal).
    Each line is either:
        <id>(<cap>)                -> leaf
        (<left_len> <right_len> <k>) -> internal or inverter (k=0 or 1)
    """
    with open(filename) as f:
        lines = [ln.strip() for ln in f if ln.strip()]

    stack = []
    nodes = {}
    edges = []
    node_id = 0

    for line in lines:
        # Leaf: "2(3.5e-14)"
        m_leaf = re.match(r"(\d+)\(([^)]+)\)", line)
        if m_leaf and not line.startswith("("):
            label = int(m_leaf.group(1))
            cap = float(m_leaf.group(2))
            nid = f"L{label}"
            nodes[nid] = {"cap": cap, "label": str(label), "k": 0}
            stack.append(nid)
            continue

        # Internal/inverter: "(lenL lenR k)"
        m_node = re.match(r"\(([^ ]+)\s+([^ ]+)(?:\s+(\d+))?\)", line)
        if not m_node:
            print(f"⚠️  Skipping unrecognized line: {line}")
            continue

        left_len = float(m_node.group(1))
        right_len = float(m_node.group(2))
        k = int(m_node.group(3)) if m_node.group(3) else 0

        # Pop children (right then left, since postorder)
        right = stack.pop() if stack else None
        left = stack.pop() if stack else None

        node_id += 1
        nid = f"N{node_id}"
        nodes[nid] = {"left_len": left_len, "right_len": right_len, "k": k}

        if left and left_len >= 0:
            edges.append((nid, left, left_len))
        if right and right_len >= 0:
            edges.append((nid, right, right_len))

        stack.append(nid)

    root = stack[-1] if stack else None
    return nodes, edges, root


def draw_pa1_tree(nodes, edges, root, title=None):
    G = nx.DiGraph()
    for src, dst, length in edges:
        G.add_edge(src, dst, length=length)

    pos = nx.nx_agraph.graphviz_layout(G, prog="dot")

    edge_labels = {(u, v): f"{d['length']:.1e}" for u, v, d in G.edges(data=True)}

    colors, labels = [], {}
    for n in G.nodes():
        k = nodes[n].get("k", 0)
        if n.startswith("L"):
            cap = nodes[n].get("cap", None)
            lbl = nodes[n].get("label", n)
            labels[n] = f"{lbl}\n{cap:.2e} F" if cap is not None else lbl
            colors.append("#a8f7a2")  # leaf = green
        elif k == 1:
            labels[n] = "INV"
            colors.append("#f5a277")  # inverter = orange
        else:
            labels[n] = n
            colors.append("#7fb7f7")  # internal = blue

    plt.figure(figsize=(12, 8))
    nx.draw(G, pos, labels=labels, node_color=colors,
            node_size=1800, arrows=False, font_size=8)
    nx.draw_networkx_edge_labels(G, pos, edge_labels=edge_labels, font_size=7)
    plt.title(title or f"PA1 Tree (root={root})")
    plt.tight_layout()
    plt.show()
    plt.savefig("pa1_tree.png", dpi=300)


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python visualize_pa1_btopo_caps.py <file.btopo>")
        sys.exit(1)

    fname = sys.argv[1]
    nodes, edges, root = parse_pa1_btopo(fname)
    print(f"✅ Parsed {len(nodes)} nodes, {len(edges)} edges, root={root}")
    draw_pa1_tree(nodes, edges, root, title=f"Tree from {fname}")
