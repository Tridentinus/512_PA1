#include "tree.h"
#include <cmath>
Node * buildTree(FILE * in) {
    char buf[256];

    std::stack<Node*> st;

    while (fgets(buf, sizeof(buf),in)) {
        if (buf[0] == '(') {
            double l,r;
            sscanf(buf,"(%le %le)",&l,&r);
            Node * right = st.top(); st.pop();
            Node * left = st.top(); st.pop();
            Node * node = new Node(l,r,left,right);
            left->set_parent(node, l);
            right->set_parent(node, r);
            st.push(node);
        }
        else {
            int label;
            double cap;
            sscanf(buf,"%d(%le)",&label,&cap);
            Node * node = new Node(label,cap);
            st.push(node);
        }
    }
    Node * root = st.top();
    root->set_parent(nullptr, 0.0);
    return root;
}

void preorder(Node * root, FILE *out) {
    if (!root) return;

    if (!root->leaf()) {
        printf("Internal node: left_len=%.10le, right_len=%.10le, parent_len=%.10le\n", root->left_len(), root->right_len(), root->parent_len());
        fprintf(out, "(%.10le %.10le)\n",root->left_len(),root->right_len());
        preorder(root->left(),out);
        preorder(root->right(),out);
    } else{
        printf("Leaf node: label=%d, cap=%.10le, parent_len=%.10le\n", root->label(), root->cap(), root->parent_len());
        fprintf(out, "%d(%.10le)\n",root->label(),root->cap());
    }
}

bool has_left(const Node*n) {return n->left() && n->left_len() >=0;};

bool has_right(const Node*n) {return n->right() && n->right_len() >=0;};

void build_c_prime(Node * node, double Co, double c,bool is_root) {
  if (is_root) {
      node->add_c_prime(Co);
  }  
  if (node->leaf()) {
    node->add_c_prime(node->cap());
  } else {
    if (has_left(node)) {
        double LCe = c * node->left_len();
        node->add_c_prime(LCe/2);
        node->left()->add_c_prime(LCe/2);
    }
    if (has_right(node)) {
        double RCe = c * node->right_len();
        node->add_c_prime(RCe/2);
        node->right()->add_c_prime(RCe/2);
    }
    if (has_left(node)) {  build_c_prime(node->left(),Co,c);};
    if (has_right(node)) {  build_c_prime(node->right(),Co,c);};
    }
}

void build_c_prime_dp(Node * root, double Co, double c, bool is_root) {
    if (!root) return;
    
    std::stack<Node*> stack;
    stack.push(root);

    if (is_root) {
        root->add_c_prime(Co);
    }
    
    while (!stack.empty()) {
        Node* node = stack.top();
        stack.pop();
        
        if (node->leaf()) {
            node->add_c_prime(node->cap());
        } 
        else {
          if (has_left(node)) {
              double LCe = c * node->left_len();
              node->add_c_prime(LCe/2);
              node->left()->add_c_prime(LCe/2);
              stack.push(node->left()); 
          }
          if (has_right(node)) {
              double RCe = c * node->right_len();
              node->add_c_prime(RCe/2);
              node->right()->add_c_prime(RCe/2);
              stack.push(node->right());  
          }
        }
    }
}

double recur_downstream(Node * node) {
  if (!node) return 0.0;
  double S = node->c_prime();
  if (!node->leaf()) {
    double lDown = recur_downstream(node->left());
    double rDown = recur_downstream(node->right());
    S += lDown + rDown;
  }
  node->set_c_downstream(S);
  return S;
}

void dp_downstream(Node * root) {
  if (!root) return;
  std::stack<Node*> stack,post;

  stack.push(root);
  while (!stack.empty()) {
    Node * u = stack.top(); stack.pop();
    post.push(u);
    if (!u->leaf()) {
      stack.push(u->left());
      stack.push(u->right());
    }
  }

  while (!post.empty()) {
    Node * v = post.top(); post.pop();
    double S = v->c_prime();
    if (!v->leaf()) {
        double lDown = v->left()->c_downstream();
        double rDown = v->right()->c_downstream();
        S += lDown + rDown;
    }
    v->set_c_downstream(S);
  }

}

void recur_delay(Node * node, double pDelay, double pResistance,double r,FILE*out) {
  double t_node = pDelay + (pResistance * node->c_downstream());
  node->set_elmore_delay(t_node);
  if (!node->leaf()) {
    recur_delay(node->left(),t_node, r* node->left_len(), r,out);
    recur_delay(node->right(),t_node, r* node->right_len(), r,out);
  } 
  else {
    int l[1] = {node->label()};
    fwrite(l,sizeof(int),1,out);
    double t[1] = {t_node};
    fwrite(t,sizeof(double),1,out);
  }
}

void dp_delay (Node * root, double Rb,double re, FILE * out) {
  if(!root) return;

  const double driver = Rb * root->c_downstream();

  std::stack<std::pair<Node*, double>> st;

  st.push({root,0.0});

  while (!st.empty()) {
    Node * node; double acc;
    {auto t = st.top(); st.pop(); node = t.first; acc = t.second;}


    node->set_elmore_delay(driver + acc);
    if (!node->leaf()) {
      Node*r = node->right();
      double acc_r = acc + node->right_len()*re * r->c_downstream();
      st.push({r,acc_r});
      Node*l = node->left();
      double acc_l = acc + node->left_len()*re * l->c_downstream();      st.push({l,acc_l});

    }
    else {
      int l[1] = {node->label()};
      fwrite(l,sizeof(int),1,out);
      double t[1] = {node->elmore_delay()};
      fwrite(t,sizeof(double),1,out);
    }
  }
}

// Helper to compute max distance for hypothetical check
double compute_max_distance_hyp(double CT, double Tmax, double T_constraint, 
                                double Rb, double r, double c, double Co) {
    double a = r * c / 2.0;
    double b = Rb * c + r * CT;
    double c_coef = Rb * Co + Rb * CT + Tmax - T_constraint;
    
    double discriminant = b * b - 4.0 * a * c_coef;
    
    if (discriminant < 0) {
        return 0.0;
    }
    
    double L_max = (-b + sqrt(discriminant)) / (2.0 * a);
    return (L_max > 0.0) ? L_max : 0.0;
}

// Insert single inverter between node and its parent
Node* insert_inverter(Node* node, double L_max, double Rb, double r, double c, 
                      double Co, double Cb, double Tb) {
    double L_parent = node->parent_len();
    Node* grandparent = node->parent();  // GET GRANDPARENT FIRST!
    
    // Create inverter node
    Node* inv_node = new Node(L_max, -1.0, node, nullptr);
    inv_node->set_k(1);
    inv_node->set_parent(grandparent, L_parent - L_max);
    
    // Update node's parent
    node->set_parent(inv_node, L_max);
    
    // Update grandparent's child pointer AND edge length
    if (grandparent) {
        if (grandparent->left() == node) {
            grandparent->set_left(inv_node);
            grandparent->set_left_len(L_parent - L_max);  // ADD THIS!
        } else {
            grandparent->set_right(inv_node);
            grandparent->set_right_len(L_parent - L_max);  // ADD THIS!
        }
    }
    
    return inv_node;
}

// Insert repeater (2 inverters) between node and its parent
std::pair<Node*, Node*> insert_repeater(Node* node, double L_max, 
                                        double Rb, double r, double c, 
                                        double Co, double Cb, double Tb) {
    double L_parent = node->parent_len();
    Node* grandparent = node->parent();  // GET GRANDPARENT FIRST!
    
    // Create first inverter (closer to node)
    Node* inv1 = new Node(L_max, -1.0, node, nullptr);
    inv1->set_k(1);
    
    // Create second inverter (closer to parent)
    Node* inv2 = new Node(0.0, -1.0, inv1, nullptr);
    inv2->set_k(1);
    
    // Setup connections
    inv2->set_parent(grandparent, L_parent - L_max);
    inv1->set_parent(inv2, 0.0);
    node->set_parent(inv1, L_max);
    
    // Update grandparent's child pointer AND edge length
    if (grandparent) {
        if (grandparent->left() == node) {
            grandparent->set_left(inv2);
            grandparent->set_left_len(L_parent - L_max);  // ADD THIS!
        } else {
            grandparent->set_right(inv2);
            grandparent->set_right_len(L_parent - L_max);  // ADD THIS!
        }
    }
    
    return {inv1, inv2};
}

// Insert inverter at top of child branch (for parity mismatch / root issues)
Node* insert_inverter_on_child(Node* parent, int child_side,
                                double Rb, double r, double c, 
                                double Co, double Cb, double Tb) {
    Node* child;
    double edge_len;
    
    if (child_side == 0) {
        child = parent->left();
        edge_len = parent->left_len();
    } else {
        child = parent->right();
        edge_len = parent->right_len();
    }
    
    Node* inv_node = new Node(0.0, -1.0, child, nullptr);
    inv_node->set_k(1);
    inv_node->set_parent(parent, edge_len);
    
    child->set_parent(inv_node, 0.0);
    
    // Update parent's child pointer
    if (child_side == 0) {
        parent->set_left(inv_node);
    } else {
        parent->set_right(inv_node);
    }
    
    return inv_node;
}

// Main recursive processing function
void process_node(Node* node, double T_constraint, 
                  double Rb, double r, double c, double Co, double Cb, double Tb) {
    
    if (!node) return;
    
    double CT_down, Tmax_down;
    int parity_down;
    
    // ===== LEAF NODE =====
    if (node->leaf()) {
        CT_down = node->cap();
        Tmax_down = 0.0;
        parity_down = 1;  // Leaves start odd
        
        node->set_parity(parity_down);
        
        
        
        // Compute upstream state
        double L_parent = node->parent_len();
        node->set_c_upstream(CT_down + c * L_parent);
        node->set_t_upstream(r * L_parent * (c * L_parent / 2.0 + CT_down) + Tmax_down);
        
        // Check hypothetical
        double t_hyp = Rb * (Co + c * L_parent + CT_down) + 
                       r * L_parent * (c * L_parent / 2.0 + CT_down) + Tmax_down;
        
        if (t_hyp > T_constraint) {
            // Need inverter on edge to parent
            double L_max = compute_max_distance_hyp(CT_down, Tmax_down, T_constraint, 
                                                    Rb, r, c, Co);
            
            if (parity_down == 1) {
                // Odd parity - single inverter
                Node* inv = insert_inverter(node, L_max, Rb, r, c, Co, Cb, Tb);
                process_node(inv, T_constraint, Rb, r, c, Co, Cb, Tb);
            } else {
                // Even parity - repeater
                auto [inv1, inv2] = insert_repeater(node, L_max, Rb, r, c, Co, Cb, Tb);
                process_node(inv1, T_constraint, Rb, r, c, Co, Cb, Tb);
                process_node(inv2, T_constraint, Rb, r, c, Co, Cb, Tb);
            }
        }
        
        return;
    }
    
    // ===== INVERTER NODE =====
    if (node->k() > 0) {
        // Process child first
        process_node(node->left(), T_constraint, Rb, r, c, Co, Cb, Tb);
        
        // Compute inverter output
        CT_down = Cb * node->k();
        Tmax_down = Tb + Rb / node->k();
        parity_down = 1 - node->left()->parity();  // Flip parity
        
        node->set_parity(parity_down);
        
        // If this is root, we're done
        if (!node->parent()) {
            return;
        }
        
        // Compute upstream state
        double L_parent = node->parent_len();
        node->set_c_upstream(CT_down + c * L_parent);
        node->set_t_upstream(r * L_parent * (c * L_parent / 2.0 + CT_down) + Tmax_down);
        
        // Check hypothetical
        double t_hyp = Rb * (Co + c * L_parent + CT_down) + 
                       r * L_parent * (c * L_parent / 2.0 + CT_down) + Tmax_down;
        
        if (t_hyp > T_constraint) {
            double L_max = compute_max_distance_hyp(CT_down, Tmax_down, T_constraint, 
                                                    Rb, r, c, Co);
            
            if (parity_down == 1) {
                Node* inv = insert_inverter(node, L_max, Rb, r, c, Co, Cb, Tb);
                process_node(inv, T_constraint, Rb, r, c, Co, Cb, Tb);
            } else {
                auto [inv1, inv2] = insert_repeater(node, L_max, Rb, r, c, Co, Cb, Tb);
                process_node(inv1, T_constraint, Rb, r, c, Co, Cb, Tb);
                process_node(inv2, T_constraint, Rb, r, c, Co, Cb, Tb);
            }
        }
        
        return;
    }
    
    // ===== INTERNAL NODE =====
    // Process both children first
    process_node(node->left(), T_constraint, Rb, r, c, Co, Cb, Tb);
    process_node(node->right(), T_constraint, Rb, r, c, Co, Cb, Tb);
    
    // Check parity mismatch
    int parity_left = node->left()->parity();
    int parity_right = node->right()->parity();
    
    if (parity_left != parity_right) {
        // Insert inverter on odd child
        if (parity_left == 1) {
            Node* inv = insert_inverter_on_child(node, 0, Rb, r, c, Co, Cb, Tb);
            process_node(inv, T_constraint, Rb, r, c, Co, Cb, Tb);
        } else {
            Node* inv = insert_inverter_on_child(node, 1, Rb, r, c, Co, Cb, Tb);
            process_node(inv, T_constraint, Rb, r, c, Co, Cb, Tb);
        }
        
        // Recompute after fixing parity
        parity_left = node->left()->parity();
        parity_right = node->right()->parity();
    }
    
    // Compute internal node state
    double C_left = node->left()->c_upstream();
    double C_right = node->right()->c_upstream();
    double T_left = node->left()->t_upstream();
    double T_right = node->right()->t_upstream();
    
    CT_down = C_left + C_right;
    Tmax_down = (T_left > T_right) ? T_left : T_right;
    parity_down = parity_left;  // Both same now
    
    node->set_parity(parity_down);
    
    // ===== ROOT CASE =====
    if (!node->parent()) {
        // Check driver capacity
        double delay = Rb * (Co + CT_down) + Tmax_down;
        
        if (delay > T_constraint || parity_down == 1) {
            // Insert inverters on both branches
            Node* inv_left = insert_inverter_on_child(node, 0, Rb, r, c, Co, Cb, Tb);
            Node* inv_right = insert_inverter_on_child(node, 1, Rb, r, c, Co, Cb, Tb);
            
            process_node(inv_left, T_constraint, Rb, r, c, Co, Cb, Tb);
            process_node(inv_right, T_constraint, Rb, r, c, Co, Cb, Tb);
        }
        
        node->set_k(1);  // Root always has k=1
        return;
    }
    
    // ===== NON-ROOT INTERNAL NODE =====
    // Compute upstream state
    double L_parent = node->parent_len();
    node->set_c_upstream(CT_down + c * L_parent);
    node->set_t_upstream(r * L_parent * (c * L_parent / 2.0 + CT_down) + Tmax_down);
    
    // Check hypothetical
    double t_hyp = Rb * (Co + c * L_parent + CT_down) + 
                   r * L_parent * (c * L_parent / 2.0 + CT_down) + Tmax_down;
    
    if (t_hyp > T_constraint) {
        double L_max = compute_max_distance_hyp(CT_down, Tmax_down, T_constraint, 
                                                Rb, r, c, Co);
        
        if (parity_down == 1) {
            // Single inverter
            Node* inv = insert_inverter(node, L_max, Rb, r, c, Co, Cb, Tb);
            process_node(inv, T_constraint, Rb, r, c, Co, Cb, Tb);
        } else {
            // Repeater
            auto [inv1, inv2] = insert_repeater(node, L_max, Rb, r, c, Co, Cb, Tb);
            process_node(inv1, T_constraint, Rb, r, c, Co, Cb, Tb);
            process_node(inv2, T_constraint, Rb, r, c, Co, Cb, Tb);
        }
    }
}

// Main entry point
void inverter_insertion(Node* root, double T_constraint, 
                        double Rb, double r, double c, double Co, double Cb, double Tb) {
    process_node(root, T_constraint, Rb, r, c, Co, Cb, Tb);
}

void postorder_with_inverters(Node* root, FILE* out) {
    if (!root) return;
    
    // Post-order: process children first, then self
    
    if (!root->leaf()) {
        // Internal or inverter node - process children
        if (root->left()) {
            postorder_with_inverters(root->left(), out);
        }
        if (root->right()) {
            postorder_with_inverters(root->right(), out);
        }
        
        // Print this node
        // Format: (left_len right_len k)
        // If right child doesn't exist, use -1.0 for right_len
        double left_len = root->left_len();
        double right_len = root->right() ? root->right_len() : -1.0;
        int k = root->k();
        
        fprintf(out, "(%.10le %.10le %d)\n", left_len, right_len, k);
    } else {
        // Leaf node
        // Format: label(capacitance)
        fprintf(out, "%d(%.10le)\n", root->label(), root->cap());
    }
}

// Post-order traversal output for topology with inverters (BINARY)
void postorder_with_inverters_binary(Node* root, FILE* out) {
    if (!root) return;
    
    // Post-order: process children first, then self
    
    if (!root->leaf()) {
        // Internal or inverter node - process children
        if (root->left()) {
            postorder_with_inverters_binary(root->left(), out);
        }
        if (root->right()) {
            postorder_with_inverters_binary(root->right(), out);
        }
        
        // Write this node in binary format
        // Format: int(-1), double(left_len), double(right_len), int(k)
        int marker = -1;
        double left_len = root->left_len();
        double right_len = root->right() ? root->right_len() : -1.0;
        int k = root->k();
        
        fwrite(&marker, sizeof(int), 1, out);
        fwrite(&left_len, sizeof(double), 1, out);
        fwrite(&right_len, sizeof(double), 1, out);
        fwrite(&k, sizeof(int), 1, out);
    } else {
        // Leaf node
        // Format: int(label), double(capacitance)
        int label = root->label();
        double cap = root->cap();
        
        fwrite(&label, sizeof(int), 1, out);
        fwrite(&cap, sizeof(double), 1, out);
    }
}
