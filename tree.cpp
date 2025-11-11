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

Node* insert_inverter(Node* node, double L_max, double Rb, double r, double c, double Co, double Cb, double Tb) {
    double L_parent = node->parent_len();
    Node* grandparent = node->parent();
    
    
    if (L_max > L_parent) {
        L_max = L_parent;
    }
    
    Node* inv_node = new Node(L_max, -1.0, node, nullptr);
    inv_node->set_k(1);
    inv_node->set_parent(grandparent, L_parent - L_max);
    
    
    node->set_parent(inv_node, L_max);
    
    if (grandparent) {
        if (grandparent->left() == node) {
            grandparent->set_left(inv_node);
            grandparent->set_left_len(L_parent - L_max);
        } else {
            grandparent->set_right(inv_node);
            grandparent->set_right_len(L_parent - L_max);
        }
    }
    
    return inv_node;
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
    
    
    Node* inv_node = new Node(0.0, -1.0, child, nullptr);
    inv_node->set_k(1);
    inv_node->set_parent(parent, edge_len);
    
    
    child->set_parent(inv_node, 0.0);
    
    if (child_side == 0) {
        parent->set_left(inv_node);
    } else {
        parent->set_right(inv_node);
    }
    
    return inv_node;
}

double compute_max_distance_hyp(double CT, double Tmax, double T_constraint, double Rb, double r, double c, double Co) {
    
    double a = r * c / 2.0;
    double b = Rb * c + r * CT;
    double c_coef = Rb * Co + Rb * CT + Tmax - T_constraint;
    
    
    double discriminant = b * b - 4.0 * a * c_coef;
    
    
    if (discriminant < 0) {
        return -1;
    }
    
    double L_max = (-b + sqrt(discriminant)) / (2.0 * a);
    
    
    return L_max;
}

bool process_node(Node* node, double T_constraint, double Rb, double r, double c, double Co, double Cb, double Tb) {
    
    if (!node) return true;
    static int depth = 0;
    depth++;
    double CT_down, Tmax_down;
    int parity_down;
    static int zero_insert_streak = 0;

    // ===== LEAF NODE =====
    if (node->leaf()) {
        CT_down = node->cap();
        Tmax_down = 0.0;
        parity_down = 1; 
        node->set_parity(parity_down);

        
        double L_parent = node->parent_len();
        double c_up = CT_down + c * L_parent;
        double t_up = r * L_parent * (c * L_parent / 2.0 + CT_down) + Tmax_down;
        node->set_c_upstream(c_up);
        node->set_t_upstream(t_up);


        double t_hyp = Rb * (Co + c * L_parent + CT_down) + 
                       r * L_parent * (c * L_parent / 2.0 + CT_down) + Tmax_down;
        
        if (t_hyp > T_constraint) {
            double L_max = compute_max_distance_hyp(CT_down, Tmax_down, T_constraint, Rb, r, c, Co);
           

            if (L_max < 0.0) {
                // L_max= 0.0;
                // zero_insert_streak++;

                // its cooked
                return false;
            }

            if (L_max == 0.0) {
                if (zero_insert_streak > 1) {
                    depth--;
                    return false; 
                }
            } else {
                zero_insert_streak = 0;
            }
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
        CT_down = Cb * node->k();
        Tmax_down = Tb;
        parity_down = 1 - node->left()->parity();
        node->set_parity(parity_down);
        
        // ===== ROOT CASE =====
        if (!node->parent()) {
            depth--;
            zero_insert_streak = 0;
            return true;
        }
        
        double L_parent = node->parent_len();
        double c_up = CT_down + c * L_parent;
        double t_up = r * L_parent * (c * L_parent / 2.0 + CT_down) + Tmax_down;
        node->set_c_upstream(c_up);
        node->set_t_upstream(t_up);
        
        double t_hyp = Rb * (Co + c * L_parent + CT_down) + 
                       r * L_parent * (c * L_parent / 2.0 + CT_down) + Tmax_down;
        if (t_hyp > T_constraint) {
            double L_max = compute_max_distance_hyp(CT_down, Tmax_down, T_constraint, Rb, r, c, Co);
            if (L_max < 0.0) {
                // L_max= 0.0;
                // zero_insert_streak++;

                // its over
                return false;

            }

            if (L_max == 0.0) {
                if (zero_insert_streak > 1) {
                    depth--;
                    return false;
                }
            } else {
                zero_insert_streak = 0;
            }            
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
        
        if (!process_node(node->left(), T_constraint, Rb, r, c, Co, Cb, Tb)) {
            depth--;
            return false;
        }
        if (!process_node(node->right(), T_constraint, Rb, r, c, Co, Cb, Tb)) {
            depth--;
            return false;
        }
        
        int parity_left = node->left()->parity();
        int parity_right = node->right()->parity();
        
        if (parity_left != parity_right) {
        
            if (parity_left == 1) {
                Node* inv = insert_inverter_on_child(node, 0, Rb, r, c, Co, Cb, Tb);  // 0 = left
                if (!process_node(inv, T_constraint, Rb, r, c, Co, Cb, Tb)) {
                    depth--;
                    return false;
                }
            } else {
                Node* inv = insert_inverter_on_child(node, 1, Rb, r, c, Co, Cb, Tb);  // 1 = right
                if (!process_node(inv, T_constraint, Rb, r, c, Co, Cb, Tb)) {
                    depth--;
                    return false;
                }
            }
            
            parity_left = node->left()->parity();
            parity_right = node->right()->parity();
        }
        
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
            
            double delay = Rb * (Co + CT_down) + Tmax_down;
            
            if (delay > T_constraint || parity_down == 1) {
                
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
            
            node->set_k(1);
            depth--;
            zero_insert_streak = 0;
            return true;
        }
        
        // ===== NON-ROOT INTERNAL NODE =====
        double L_parent = node->parent_len();
        double c_up = CT_down + c * L_parent;
        double t_up = r * L_parent * (c * L_parent / 2.0 + CT_down) + Tmax_down;
        node->set_c_upstream(c_up);
        node->set_t_upstream(t_up);
        
        double t_hyp = Rb * (Co + c * L_parent + CT_down) + 
                       r * L_parent * (c * L_parent / 2.0 + CT_down) + Tmax_down;
        
        
        if (t_hyp > T_constraint) {
            double L_max = compute_max_distance_hyp(CT_down, Tmax_down, T_constraint, Rb, r, c, Co);
            
            if (L_max < 0.0) {
                // L_max= 0.0;

                return false;
            }
            if (L_max == 0.0) {
                zero_insert_streak++;
                if (zero_insert_streak > 1) {
                    depth--;
                    return false;  
                }
            } else {
                zero_insert_streak = 0;
            }

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

bool inverter_insertion(Node* root, double T_constraint, double Rb, double r, double c, double Co, double Cb, double Tb) {
    return process_node(root, T_constraint, Rb, r, c, Co, Cb, Tb);
}

void postorder_with_inverters(Node* root, FILE* out) {
    if (!root) return;
    
    
    if (!root->leaf()) {
        if (root->left()) {
            postorder_with_inverters(root->left(), out);
        }
        if (root->right()) {
            postorder_with_inverters(root->right(), out);
        }
        
        
        double left_len = root->left_len();
        double right_len = root->right() ? root->right_len() : -1.0;
        int k = root->k();
        
        fprintf(out, "(%.10le %.10le %d)\n", left_len, right_len, k);
    } else {
        fprintf(out, "%d(%.10le)\n", root->label(), root->cap());
    }
}

void postorder_with_inverters_binary(Node* root, FILE* out) {
    if (!root) return;
    
    
    if (!root->leaf()) {
        if (root->left()) {
            postorder_with_inverters_binary(root->left(), out);
        }
        if (root->right()) {
            postorder_with_inverters_binary(root->right(), out);
        }
        int marker = -1;
        double left_len = root->left_len();
        double right_len = root->right() ? root->right_len() : -1.0;
        int k = root->k();
        
        fwrite(&marker, sizeof(int), 1, out);
        fwrite(&left_len, sizeof(double), 1, out);
        fwrite(&right_len, sizeof(double), 1, out);
        fwrite(&k, sizeof(int), 1, out);
    } else {
        int label = root->label();
        double cap = root->cap();
        
        fwrite(&label, sizeof(int), 1, out);
        fwrite(&cap, sizeof(double), 1, out);
    }
}

