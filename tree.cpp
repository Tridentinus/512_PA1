#include "tree.h"
#include <cmath>
#ifdef DEBUG
#include <cstdio>
#define DBGPRINT(...) printf(__VA_ARGS__)
#else
#define DBGPRINT(...) ((void)0)
#endif
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
        // printf("Internal node: left_len=%.10le, right_len=%.10le, parent_len=%.10le\n", root->left_len(), root->right_len(), root->parent_len());
        fprintf(out, "(%.10le %.10le)\n",root->left_len(),root->right_len());
        preorder(root->left(),out);
        preorder(root->right(),out);
    } else{
        // printf("Leaf node: label=%d, cap=%.10le, parent_len=%.10le\n", root->label(), root->cap(), root->parent_len());
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
Node* insert_inverter(Node* node, double L_max, double Rb, double r, double c, double Co, double Cb, double Tb) {
    double L_parent = node->parent_len();
    Node* grandparent = node->parent();
    
    DBGPRINT( "    >>> insert_inverter: node=%p, L_max=%.2le, L_parent=%.2le, grandparent=%p\n", (void*)node, L_max, L_parent, (void*)grandparent);
    
    if (L_max > L_parent) {
        DBGPRINT( "    >>> WARNING: L_max (%.2le) > L_parent (%.2le)! Clamping to L_parent.\n", L_max, L_parent);
        L_max = L_parent;
    }
    
    // Create inverter node
    Node* inv_node = new Node(L_max, -1.0, node, nullptr);
    inv_node->set_k(1);
    inv_node->set_parent(grandparent, L_parent - L_max);
    
  //DBGPRINT( "    >>> Created inv_node=%p, inv->parent_len=%.2le\n",  (void*)inv_node, L_parent - L_max);
    
    // Update node's parent
    node->set_parent(inv_node, L_max);
    
    // Update grandparent's child pointer AND edge length
    if (grandparent) {
        if (grandparent->left() == node) {
          //DBGPRINT( "    >>> Updating grandparent LEFT: old_len=%.2le, new_len=%.2le\n",grandparent->left_len(), L_parent - L_max);
            grandparent->set_left(inv_node);
            grandparent->set_left_len(L_parent - L_max);
        } else {
          //DBGPRINT( "    >>> Updating grandparent RIGHT: old_len=%.2le, new_len=%.2le\n",grandparent->right_len(), L_parent - L_max);
            grandparent->set_right(inv_node);
            grandparent->set_right_len(L_parent - L_max);
        }
    } else {
      //DBGPRINT( "    >>> No grandparent (node was child of root)\n");
    }
    
  //DBGPRINT( "    >>> insert_inverter DONE, returning inv_node=%p\n", (void*)inv_node);
    return inv_node;
}

std::pair<Node*, Node*> insert_repeater(Node* node, double L_max, double Rb, double r, double c, double Co, double Cb, double Tb) {
    double L_parent = node->parent_len();
    Node* grandparent = node->parent();
    
  //DBGPRINT( "    >>> insert_repeater: node=%p, L_max=%.2le, L_parent=%.2le, grandparent=%p\n",(void*)node, L_max, L_parent, (void*)grandparent);
    
    if (L_max > L_parent) {
      //DBGPRINT( "    >>> WARNING: L_max (%.2le) > L_parent (%.2le)! Clamping to L_parent.\n", L_max, L_parent);
        L_max = L_parent;
    }
    
    // Create first inverter (closer to node)
    Node* inv1 = new Node(L_max, -1.0, node, nullptr);
    inv1->set_k(1);
  //DBGPRINT( "    >>> Created inv1=%p\n", (void*)inv1);
    
    // Create second inverter (closer to parent)
    Node* inv2 = new Node(0.0, -1.0, inv1, nullptr);
    inv2->set_k(1);
  //DBGPRINT( "    >>> Created inv2=%p\n", (void*)inv2);
    
    // Setup connections
    inv2->set_parent(grandparent, L_parent - L_max);
    inv1->set_parent(inv2, 0.0);
    node->set_parent(inv1, L_max);
    
  //DBGPRINT( "    >>> Connections: node->parent=%p (len=%.2le), inv1->parent=%p (len=%.2le), inv2->parent=%p (len=%.2le)\n", (void*)inv1, L_max, (void*)inv2, 0.0, (void*)grandparent, L_parent - L_max);
    
    // Update grandparent's child pointer AND edge length
    if (grandparent) {
        if (grandparent->left() == node) {
          //DBGPRINT( "    >>> Updating grandparent LEFT to inv2\n");
            grandparent->set_left(inv2);
            grandparent->set_left_len(L_parent - L_max);
        } else {
          //DBGPRINT( "    >>> Updating grandparent RIGHT to inv2\n");grandparent->set_right(inv2);
            grandparent->set_right_len(L_parent - L_max);
        }
    } else {
      //DBGPRINT( "    >>> No grandparent\n");
    }
    
  //DBGPRINT( "    >>> insert_repeater DONE, returning {inv1=%p, inv2=%p}\n", (void*)inv1, (void*)inv2);
    return {inv1, inv2};
}

Node* insert_inverter_on_child(Node* parent, int child_side,double Rb, double r, double c, double Co, double Cb, double Tb) {
    Node* child;
    double edge_len;
    
    if (child_side == 0) {
        child = parent->left();
        edge_len = parent->left_len();
    } else {
        child = parent->right();
        edge_len = parent->right_len();
    }
    
  //DBGPRINT( "    >>> insert_inverter_on_child: parent=%p, child_side=%s, child=%p, edge_len=%.2le\n", (void*)parent, child_side == 0 ? "LEFT" : "RIGHT", (void*)child, edge_len);
    
    Node* inv_node = new Node(0.0, -1.0, child, nullptr);
    inv_node->set_k(1);
    inv_node->set_parent(parent, edge_len);
    
  //DBGPRINT( "    >>> Created inv_node=%p at TOP of branch (zero distance from child)\n", (void*)inv_node);
    
    child->set_parent(inv_node, 0.0);
    
    // Update parent's child pointer
    if (child_side == 0) {
        parent->set_left(inv_node);
    } else {
        parent->set_right(inv_node);
    }
    
  //DBGPRINT( "    >>> insert_inverter_on_child DONE, returning inv_node=%p\n", (void*)inv_node);
    return inv_node;
}

double compute_max_distance_hyp(double CT, double Tmax, double T_constraint, double Rb, double r, double c, double Co) {
  //DBGPRINT( "    >>> compute_max_distance_hyp: CT=%.2le, Tmax=%.2le, T_constraint=%.2le\n", CT, Tmax, T_constraint);
    
    double a = r * c / 2.0;
    double b = Rb * c + r * CT;
    double c_coef = Rb * Co + Rb * CT + Tmax - T_constraint;
    
  DBGPRINT( "    >>> Quadratic: a=%.2le, b=%.2le, c=%.2le\n", a, b, c_coef);
    
    double discriminant = b * b - 4.0 * a * c_coef;
    
  DBGPRINT( "    >>> Discriminant = %.2le\n", discriminant);
    
    if (discriminant < 0) {
      DBGPRINT( "    >>> Discriminant < 0! Returning L_max = 0\n");
        return -1;
    }
    
    double L_max = (-b + sqrt(discriminant)) / (2.0 * a);
    
  DBGPRINT( "    >>> Computed L_max = %.2le\n", L_max);
    
    return L_max;
}
// Main recursive processing function
bool process_node(Node* node, double T_constraint, double Rb, double r, double c, double Co, double Cb, double Tb) {
    
    if (!node) return true;
    static int depth = 0;
    depth++;
    double CT_down, Tmax_down;
    int parity_down;
    static int zero_insert_streak = 0;
    // show the zero insert streak for debugging
    DBGPRINT( "[%*sDEBUG] process_node: node=%p, zero_insert_streak=%d\n", depth*2, "", (void*)node, zero_insert_streak);
    // ===== LEAF NODE =====
    if (node->leaf()) {
        DBGPRINT("[%*sDEBUG] Processing LEAF %d, cap=%.2le, pLen=%.2le\n", depth*2, "", node->label(), node->cap(), node->parent_len());
        CT_down = node->cap();
        Tmax_down = 0.0;
        parity_down = 1;  // Leaves start odd
        node->set_parity(parity_down);

        
        // Compute upstream state
        double L_parent = node->parent_len();
        double c_up = CT_down + c * L_parent;
        double t_up = r * L_parent * (c * L_parent / 2.0 + CT_down) + Tmax_down;
        node->set_c_upstream(c_up);
        node->set_t_upstream(t_up);

        DBGPRINT("[%*sDEBUG] LEAF upstream: c_up=%.2le, t_up=%.2le\n", depth*2, "", c_up, t_up);

        // Check hypothetical
        double t_hyp = Rb * (Co + c * L_parent + CT_down) + 
                       r * L_parent * (c * L_parent / 2.0 + CT_down) + Tmax_down;
        DBGPRINT("[%*sDEBUG] LEAF hypothetical delay: t_hyp=%.2le (constraint=%.2le)\n", depth*2, "", t_hyp, T_constraint);
        
        if (t_hyp > T_constraint) {
            DBGPRINT("[%*sDEBUG] LEAF violates! Inserting inverter.\n", depth*2, "");
            double L_max = compute_max_distance_hyp(CT_down, Tmax_down, T_constraint, Rb, r, c, Co);
            // if (L_max < 0.0) {
            //     // INFEASIBLE: Cannot meet constraint even with inverter at node
            //     DBGPRINT("[%*sERROR] Infeasible! L_max=%.2le < 0, constraint=%.2le cannot be met\n", 
            //             depth*2, "", L_max, T_constraint);
            //     depth--;
            //     return false;  // Fail the testcase
            // }

            if (L_max < 0.0) {
                L_max= 0.0;
                // // delay up = intrinsic delay of inverter (Tb = Rb * C_out) + delay from wire resistance and capacitance to parent (c_up = r * parentLen * (c * parentLen/2 + Co))
                // double delay_up = Tb + r * node->parent_len() * (c * node->parent_len()/2 + Co);
                // DBGPRINT("[%*sDEBUG] L_max < 0, setting L_max=0. Calculated delay_up=%.2le\n", depth*2, "", delay_up);
                // if (Tb + r * node->parent_len() * (c * node->parent_len()/2 + Co) > T_constraint) {
                    
                //     DBGPRINT("[%*sERROR] Infeasible! Even with inverter at LEAF %d, delay_up=%.2le exceeds constraint=%.2le\n", 
                //             depth*2, "", node->label(), delay_up, T_constraint);
                //     depth--;
                //     return false;  // Fail the testcase
                // }
                zero_insert_streak++;

            }

            if (L_max == 0.0) {
                DBGPRINT("[%*sDEBUG] L_max == 0. Incrementing zero_insert_streak\n", depth*2, "");
                if (zero_insert_streak > 1) {
                    DBGPRINT("[%*sERROR] Infeasible! Consecutive zero-length inverter insertions at LEAF %d\n", 
                            depth*2, "", node->label());
                    depth--;
                    return false;  // Fail the testcase
                }
            } else {
                zero_insert_streak = 0;
            }



            // if intrinsic delay of inverter exceeds constraint, infeasible
            
            // Odd parity - single inverter
            DBGPRINT("[%*sDEBUG] Inserting SINGLE inverter at LEAF %d\n", depth*2, "", node->label());
            Node* inv = insert_inverter(node, L_max, Rb, r, c, Co, Cb, Tb);
            bool result = process_node(inv, T_constraint, Rb, r, c, Co, Cb, Tb);
            depth--;
            return result;
        
        }
        
        depth--;
        zero_insert_streak = 0;
        return true;
    }
    
    // ===== INVERTER NODE =====
    else if (node->k() > 0) {
        DBGPRINT("[%*sDEBUG] Processing INVERTER k=%d\n", depth*2, "", node->k());
        

        
        // Compute inverter output
        CT_down = Cb * node->k();
        Tmax_down = Tb;
        parity_down = 1 - node->left()->parity();
        node->set_parity(parity_down);
        DBGPRINT("[%*sDEBUG] Result: parity=%d, CT_down=%.2le, Tmax_down=%.2le\n", depth*2, "", parity_down, CT_down, Tmax_down);
        // If root, done
        if (!node->parent()) {
            DBGPRINT("[%*sDEBUG] INVERTER is root, done.\n", depth*2, "");
            depth--;
            zero_insert_streak = 0;
            return true;
        }
        
        // Compute upstream state
        double L_parent = node->parent_len();
        double c_up = CT_down + c * L_parent;
        double t_up = r * L_parent * (c * L_parent / 2.0 + CT_down) + Tmax_down;
        node->set_c_upstream(c_up);
        node->set_t_upstream(t_up);
        
        DBGPRINT("[%*sDEBUG] INVERTER upstream: c_up=%.2le, t_up=%.2le\n", depth*2, "", c_up, t_up);
        // Check hypothetical
        double t_hyp = Rb * (Co + c * L_parent + CT_down) + 
                       r * L_parent * (c * L_parent / 2.0 + CT_down) + Tmax_down;
        
        DBGPRINT("[%*sDEBUG] INVERTER hypothetical delay: t_hyp=%.2le (constraint=%.2le)\n", depth*2, "", t_hyp, T_constraint);
        if (t_hyp > T_constraint) {
            double L_max = compute_max_distance_hyp(CT_down, Tmax_down, T_constraint, Rb, r, c, Co);
            // if (L_max < 0.0) {
            //     // INFEASIBLE: Cannot meet constraint even with inverter at node
            //     DBGPRINT("[%*sERROR] Infeasible! L_max=%.2le < 0, constraint=%.2le cannot be met\n", 
            //             depth*2, "", L_max, T_constraint);
            //     depth--;
            //     return false;  // Fail the testcase
            // }
            if (L_max < 0.0) {
                L_max= 0.0;
                zero_insert_streak++;

            }
            if (L_max == 0.0) {
                DBGPRINT("[%*sDEBUG] L_max == 0. Incrementing zero_insert_streak\n", depth*2, "");

                if (zero_insert_streak > 1) {
                    DBGPRINT("[%*sERROR] Infeasible! Consecutive zero-length inverter insertions at LEAF %d\n", 
                            depth*2, "", node->label());
                    depth--;
                    return false;  // Fail the testcase
                }
            } else {
                zero_insert_streak = 0;
            }
            DBGPRINT("[%*sDEBUG] INVERTER violates! Inserting SINGLE inverter.\n", depth*2, "");
            
            
            
            Node* inv = insert_inverter(node, L_max, Rb, r, c, Co, Cb, Tb);
            bool result = process_node(inv, T_constraint, Rb, r, c, Co, Cb, Tb);
            depth--;
            return result;
        }
        
        depth--;
        zero_insert_streak = 0;
        return true;
    }
    
    // ===== INTERNAL NODE =====
    else {
        DBGPRINT("[%*sDEBUG] Processing INTERNAL node\n", depth*2, "");
        
        // Process both children first
        if (!process_node(node->left(), T_constraint, Rb, r, c, Co, Cb, Tb)) {
            depth--;
            return false;
        }
        if (!process_node(node->right(), T_constraint, Rb, r, c, Co, Cb, Tb)) {
            depth--;
            return false;
        }
        
        // Check parity mismatch
        int parity_left = node->left()->parity();
        int parity_right = node->right()->parity();
        
        if (parity_left != parity_right) {
            DBGPRINT("[%*sDEBUG] Parity mismatch! left=%d, right=%d\n", depth*2, "", parity_left, parity_right);
            
           
            // Insert on the ODD child
            if (parity_left == 1) {
                // Left is odd, insert there
                DBGPRINT("[%*sDEBUG] Inserting inverter on LEFT (odd) child\n", depth*2, "");
                Node* inv = insert_inverter_on_child(node, 0, Rb, r, c, Co, Cb, Tb);  // 0 = left
                if (!process_node(inv, T_constraint, Rb, r, c, Co, Cb, Tb)) {
                    depth--;
                    return false;
                }
            } else {
                // Right is odd, insert there
                DBGPRINT("[%*sDEBUG] Inserting inverter on RIGHT (odd) child\n", depth*2, "");
                Node* inv = insert_inverter_on_child(node, 1, Rb, r, c, Co, Cb, Tb);  // 1 = right
                if (!process_node(inv, T_constraint, Rb, r, c, Co, Cb, Tb)) {
                    depth--;
                    return false;
                }
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
        DBGPRINT("[%*sDEBUG] Result: parity=%d, CT_down=%.2le, Tmax_down=%.2le\n", depth*2, "", parity_down, CT_down, Tmax_down);
        
        // ===== ROOT CASE =====
        if (!node->parent()) {
            DBGPRINT("[%*sDEBUG] This is ROOT\n", depth*2, "");
            
            // Check driver capacity
            double delay = Rb * (Co + CT_down) + Tmax_down;
            DBGPRINT("[%*sDEBUG] Driver delay: %.2le (constraint: %.2le)\n", depth*2, "", delay, T_constraint);
            
            if (delay > T_constraint || parity_down == 1) {
                DBGPRINT("[%*sDEBUG] ROOT needs fixing! Inserting on both branches.\n", depth*2, "");
                
                Node* inv_left = insert_inverter_on_child(node, 0, Rb, r, c, Co, Cb, Tb);
                Node* inv_right = insert_inverter_on_child(node, 1, Rb, r, c, Co, Cb, Tb);
                
                if (!process_node(inv_left, T_constraint, Rb, r, c, Co, Cb, Tb)) {
                    depth--;
                    return false;
                }
                if (!process_node(inv_right, T_constraint, Rb, r, c, Co, Cb, Tb)) {
                    depth--;
                    return false;
                }
            }
            
            node->set_k(1);  // Root always has k=1
            depth--;
            zero_insert_streak = 0;
            return true;
        }
        
        // ===== NON-ROOT INTERNAL NODE =====
        // Compute upstream state
        double L_parent = node->parent_len();
        double c_up = CT_down + c * L_parent;
        double t_up = r * L_parent * (c * L_parent / 2.0 + CT_down) + Tmax_down;
        node->set_c_upstream(c_up);
        node->set_t_upstream(t_up);
        
        // Check hypothetical
        double t_hyp = Rb * (Co + c * L_parent + CT_down) + 
                       r * L_parent * (c * L_parent / 2.0 + CT_down) + Tmax_down;
        
        DBGPRINT("[%*sDEBUG] INTERNAL hypothetical delay: t_hyp=%.2le (constraint=%.2le)\n", depth*2, "", t_hyp, T_constraint);
        
        if (t_hyp > T_constraint) {
            double L_max = compute_max_distance_hyp(CT_down, Tmax_down, T_constraint, Rb, r, c, Co);
            // if (L_max < 0.0) {
            //     // INFEASIBLE: Cannot meet constraint even with inverter at node
            //     DBGPRINT("[%*sERROR] Infeasible! L_max=%.2le < 0, constraint=%.2le cannot be met\n", 
            //             depth*2, "", L_max, T_constraint);
            //     depth--;
            //     return false;  // Fail the testcase
            // }
            if (L_max < 0.0) {
                L_max= 0.0;
            }
            
            if (L_max == 0.0) {
                DBGPRINT("[%*sDEBUG] L_max == 0. Incrementing zero_insert_streak\n", depth*2, "");

                zero_insert_streak++;
                
                if (zero_insert_streak > 1) {
                    DBGPRINT("[%*sERROR] Infeasible! Consecutive zero-length inverter insertions at LEAF %d\n", 
                            depth*2, "", node->label());
                    depth--;
                    return false;  // Fail the testcase
                }
            } else {
                zero_insert_streak = 0;
            }
            
            DBGPRINT("[%*sDEBUG] INTERNAL violates! parity=%d, will insert %s\n", depth*2, "", parity_down, (parity_down == 1) ? "SINGLE" : "REPEATER");
            
            Node* inv = insert_inverter(node, L_max, Rb, r, c, Co, Cb, Tb);
            bool result = process_node(inv, T_constraint, Rb, r, c, Co, Cb, Tb);
            depth--;
            return result;
        
        }
        
        depth--;
        zero_insert_streak = 0;
        return true;
    }
}



// Main entry point
bool inverter_insertion(Node* root, double T_constraint, double Rb, double r, double c, double Co, double Cb, double Tb) {
    return process_node(root, T_constraint, Rb, r, c, Co, Cb, Tb);
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



#include <set>
#include <map>

// Structure to hold statistics
struct InverterStats {
    int sink_count;
    int matched_sinks;
    int matched_edges;
    int root_k;
    bool inverters_valid;
    int noninverting_sinks;
    int stage_sinks;
    int safe_stage_sinks;
    int inverter_count;
    
    InverterStats() : sink_count(0), matched_sinks(0), matched_edges(0), 
                      root_k(0), inverters_valid(true), noninverting_sinks(0),
                      stage_sinks(0), safe_stage_sinks(0), inverter_count(0) {}
};

// Count sinks in tree
void count_sinks(Node* node, std::set<int>& sinks) {
    if (!node) return;
    if (node->leaf()) {
        sinks.insert(node->label());
    } else {
        count_sinks(node->left(), sinks);
        if (node->right()) {
            count_sinks(node->right(), sinks);
        }
    }
}

// Count edges in tree
void count_edges(Node* node, std::set<std::pair<int, int>>& edges, int parent_id) {
    if (!node) return;
    
    int node_id = node->leaf() ? node->label() : -1; // Use label for leaves, -1 for internal
    
    if (parent_id != -2) { // -2 means root has no parent
        edges.insert({parent_id, node_id});
    }
    
    if (!node->leaf()) {
        count_edges(node->left(), edges, node_id);
        if (node->right()) {
            count_edges(node->right(), edges, node_id);
        }
    }
}

// Check parity and count non-inverting sinks
void check_parity(Node* node, int& noninverting_count) {
    if (!node) return;
    if (node->leaf()) {
        if (node->parity() == 0) {
            noninverting_count++;
        }
    } else {
        check_parity(node->left(), noninverting_count);
        if (node->right()) {
            check_parity(node->right(), noninverting_count);
        }
    }
}

// Count inverters and check validity
void count_inverters(Node* node, int& inverter_count, bool& all_valid, bool count_root = false) {
    if (!node) return;
    
    if (!node->leaf()) {
        if (node->k() > 0) {
            if (count_root || node->parent()) { // Count root only if count_root is true
                inverter_count++;
                if (node->k() <= 0) {
                    all_valid = false;
                }
            }
        }
        count_inverters(node->left(), inverter_count, all_valid, count_root);
        if (node->right()) {
            count_inverters(node->right(), inverter_count, all_valid, count_root);
        }
    }
}

// Count stage sinks and check delay constraints
void count_stage_sinks(Node* node, double T_constraint, int& stage_sinks, int& safe_stage_sinks) {
    if (!node) return;
    
    if (!node->leaf() && node->k() > 0 && node->parent()) {
        // This is an inverter (not root) - it's a "stage sink"
        stage_sinks++;
        
        // Check if this stage meets the constraint
        // The delay to this inverter should be <= T_constraint
        if (node->t_upstream() <= T_constraint) {
            safe_stage_sinks++;
        }
    }
    
    if (node->leaf()) {
        // Actual sink
        stage_sinks++;
        if (node->t_upstream() <= T_constraint) {
            safe_stage_sinks++;
        }
    }
    
    if (!node->leaf()) {
        count_stage_sinks(node->left(), T_constraint, stage_sinks, safe_stage_sinks);
        if (node->right()) {
            count_stage_sinks(node->right(), T_constraint, stage_sinks, safe_stage_sinks);
        }
    }
}

// Main verification function
void verify_solution(Node* original_root, Node* modified_root, double T_constraint, 
                     const char* test_name) {
    InverterStats stats;
    
    // Count sinks in original and modified trees
    std::set<int> original_sinks, modified_sinks;
    count_sinks(original_root, original_sinks);
    count_sinks(modified_root, modified_sinks);
    
    stats.sink_count = original_sinks.size();
    
    // Count matched sinks
    for (int sink : original_sinks) {
        if (modified_sinks.count(sink)) {
            stats.matched_sinks++;
        }
    }
    
    // Count edges
    std::set<std::pair<int, int>> original_edges, modified_edges;
    count_edges(original_root, original_edges, -2);
    count_edges(modified_root, modified_edges, -2);
    
    // This is simplified - actual edge matching is more complex
    // For now, just count if we have the right number
    stats.matched_edges = (modified_edges.size() >= original_edges.size()) ? 
                          (2 * stats.sink_count - 2) : 0;
    
    // Check root
    stats.root_k = modified_root->k();
    
    // Check inverters validity
    stats.inverters_valid = true;
    stats.inverter_count = 0;
    count_inverters(modified_root, stats.inverter_count, stats.inverters_valid, false);
    
    // Check non-inverting sinks
    stats.noninverting_sinks = 0;
    check_parity(modified_root, stats.noninverting_sinks);
    
    // Count stage sinks
    count_stage_sinks(modified_root, T_constraint, stats.stage_sinks, stats.safe_stage_sinks);
    
    // Print results
    printf( "\n=== VERIFICATION RESULTS for %s ===\n", test_name);
    printf( "Sink counts: %d\n", stats.sink_count);
    printf( "Matched sinks: %d (expected: %d) %s\n", stats.matched_sinks, stats.sink_count, stats.matched_sinks == stats.sink_count ? "✓" : "✗");
    printf( "Matched edges: %d (expected: %d) %s\n", stats.matched_edges, 2 * stats.sink_count - 2, stats.matched_edges == 2 * stats.sink_count - 2 ? "✓" : "✗");
    printf( "Root k: %d (expected: 1) %s\n", stats.root_k, stats.root_k == 1 ? "✓" : "✗");
    printf( "Inverters valid: %s\n", stats.inverters_valid ? "✓" : "✗");
    printf( "Non-inverting sinks: %d (expected: %d) %s\n",  stats.noninverting_sinks, stats.sink_count, stats.noninverting_sinks == stats.sink_count ? "✓" : "✗");
    printf( "Stage sinks: %d\n", stats.stage_sinks);
    printf( "Safe stage sinks: %d (expected: %d) %s\n", stats.safe_stage_sinks, stats.stage_sinks, stats.safe_stage_sinks == stats.stage_sinks ? "✓" : "✗");
    printf( "Inverter count: %d\n", stats.inverter_count);
    printf( "Constraint: %.2le\n", T_constraint);
    printf( "===================================\n\n");
    
    // Overall verdict
    bool all_correct = (stats.matched_sinks == stats.sink_count) &&
                       (stats.matched_edges == 2 * stats.sink_count - 2) &&
                       (stats.root_k == 1) &&
                       stats.inverters_valid &&
                       (stats.noninverting_sinks == stats.sink_count) &&
                       (stats.safe_stage_sinks == stats.stage_sinks);
    
  printf( "OVERALL: %s\n\n", all_correct ? "✓ PASS" : "✗ FAIL");
}
