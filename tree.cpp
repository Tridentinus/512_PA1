#include "tree.h"
#include <cmath>
Node * buildTree(FILE * in) {
    char buf[256];

    std::stack<Node*> st;

    while (fgets(buf, sizeof(buf),in)) {
        if (buf[0] == '(') {
          //std::cout << "Non-leaf detected: "<< buf << std::endl;
            double l,r;
            sscanf(buf,"(%le %le)",&l,&r);
            Node * right = st.top(); st.pop();
            Node * left = st.top(); st.pop();
            Node * node = new Node(l,r,left,right);
            st.push(node);
        }
        else {
          //std::cout << "Leaf detected: "<< buf << std::endl;
            int label;
            double cap;
            sscanf(buf,"%d(%le)",&label,&cap);
            Node * node = new Node(label,cap);
            st.push(node);
        }
    }
    Node * root = st.top();
    return root;
}

void preorder(Node * root, FILE *out) {
    if (!root) return;
    if (!root->leaf()) {
      std::cout << "Non-leaf detected: " << std::endl << root->left_len() << " " <<  root->right_len() << " " << std::endl;// << root->left() << " " << root->right() << std::endl;
        fprintf(out, "(%.10le %.10le)\n",root->left_len(),root->right_len());
        preorder(root->left(),out);
        preorder(root->right(),out);
    } else{
      std::cout << "Leaf detected: " << std::endl << root->label() << " " <<  root->cap() << " " << std::endl;
        fprintf(out, "%d(%.10le)\n",root->label(),root->cap());

    }
}

bool has_left(const Node*n) {return n->left() && n->left_len() >=0;};
bool has_right(const Node*n) {return n->right() && n->right_len() >=0;};

void build_c_prime(Node * node, double Co, double r, double c,bool is_root= false) {
  if (is_root) {
      node->add_c_prime(Co);
      // std::cout << "ROOT" << std::endl;
      
  }  
  if (node->leaf()) {
      // std::cout << "Leaf detected: " << std::endl
                // << "  Starting Cap: " << node->c_prime() << std::endl;
      node->add_c_prime(node->cap());
      // std::cout << "  Sink Cap: " << node->cap() << std::endl;
      // std::cout << "  Ending Cap: " << node->c_prime() << std::endl;



    } else{
      // std::cout << "Non-leaf detected: " << std::endl
                // << "  Starting Cap: " << node->c_prime() << std::endl;
      

      if (has_left(node)) {      
        double LCe = c * node->left_len();
        node->add_c_prime(LCe/2);
        // std::cout << "  Left: "<< "(" << c << " * " << node->left_len() << ")/2" << " = " << LCe/2 << std::endl;
        node->left()->add_c_prime(LCe/2);
      }
      if (has_right(node)) {
        double RCe = c * node->right_len();
        node->add_c_prime(RCe/2);
        // std::cout << "  Right: "<< "(" << c << " * " << node->right_len() << ")/2" << " = " << RCe/2 << std::endl;
        node->right()->add_c_prime(RCe/2);
      }


      // std::cout << "  Ending Cap: " << node->c_prime() << std::endl;
      if (has_left(node)) {  build_c_prime(node->left(),Co,r,c);};
      if (has_right(node)) {  build_c_prime(node->right(),Co,r,c);};
  }

}

double recur_downstream(Node * node) {
  if (!node) return 0.0;

  double S = node->c_prime();

  if (!node->leaf()) {
    double lDown = recur_downstream(node->left());
    double rDown = recur_downstream(node->right());
    // std::cout << "Non-leaf detected: " << std::endl;
    // std::cout << "  Local Cap: " << node->c_prime() << std::endl;
    // std::cout << "  Left Cap: " << lDown << std::endl;
    // std::cout << "  Right Cap: " << rDown << std::endl;
    S += lDown + rDown;

    // std::cout << "  Total Cap: " << S << std::endl;


  }
  else {
    // std::cout << "Leaf detected: " << std::endl;
    // std::cout << "  Local Cap: " << node->c_prime() << std::endl;


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
      // std::cout << "Non-leaf detected: " << std::endl;
      // std::cout << "  Local Cap: " << v->c_prime() << std::endl;
      // std::cout << "  Left Cap: " << lDown << std::endl;
      // std::cout << "  Right Cap: " << rDown << std::endl;
      S += lDown + rDown;

      // std::cout << "  Total Cap: " << S << std::endl;
    } else {
    // std::cout << "Leaf detected: " << std::endl;
    // std::cout << "  Local Cap: " << node->c_prime() << std::endl;
    }
    v->set_c_downstream(S);
  }

}

void recur_delay(Node * node, double pDelay, double pResistance,double r,FILE*out) {
  double t_node = pDelay + (pResistance * node->c_downstream());
  node->set_elmore_delay(t_node);
  
  if (!node->leaf()) {
    std::cout << "Non-leaf detected: " << std::endl
              << "  Downstream Cap: " << node->c_downstream() <<  std::endl
              << "  Resistance from Parent: " << pResistance <<  std::endl
              << "  Delay from Parent: " << pDelay <<  std::endl
              << "  Total Delay: " << t_node << std::endl
              << "  Resistance to Left: " <<  r* node->left_len() << std::endl
              << "  Resistance to Right: " <<  r* node->right_len() << std::endl;

    recur_delay(node->left(),t_node, r* node->left_len(), r,out);
    recur_delay(node->right(),t_node, r* node->right_len(), r,out);
  } else {
    std::cout << "Leaf detected: " << std::endl
              << "  Downstream Cap: " << node->c_downstream() <<  std::endl
              << "  Resistance from Parent: " << pResistance <<  std::endl
              << "  Delay from Parent: " << pDelay <<  std::endl
              << "  Total Delay: " << t_node << std::endl;
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
      double acc_l = acc + node->left_len()*re * l->c_downstream();
      st.push({l,acc_l});

      // std::cout << "Non-leaf detected: " << std::endl
            // << "  Downstream Cap: " << node->c_downstream() <<  std::endl
            // << "  Total Delay: " << driver + acc << std::endl
            // << "  Resistance to Left: " <<  re* node->left_len() << std::endl
            // << "  Resistance to Right: " <<  re* node->right_len() << std::endl;
    }
    else {
      // std::cout << "Leaf detected: " << std::endl
              // << "  Downstream Cap: " << node->c_downstream() <<  std::endl
              // << "  Total Delay: " << driver+acc << std::endl;

              int l[1] = {node->label()};
              fwrite(l,sizeof(int),1,out);
              double t[1] = {node->elmore_delay()};
              fwrite(t,sizeof(double),1,out);
    }
  }
}

// Node * recursive_inv(Node * origin, double Rb, double r, double c, double T_max, int parent_stages) {
//   SubtreeInfo 
// }


NodeResult insert_inverters_bottom_up(Node* node, double Rb, double r, double c, 
                                      double Co, double T_constraint, bool is_root) {
    if (node->leaf()) {
        printf("[ANALYZE] Leaf node %d: cap=%.3le\n", node->label(), node->cap());
        NodeResult result;
        result.max_delay = 0.0;
        result.total_cap = node->cap();
        result.min_stages = 0;
        result.needs_inverter = 0;
        printf("  -> Returning: delay=%.3le, cap=%.3le, stages=%d\n\n",
               result.max_delay, result.total_cap, result.min_stages);
        return result;
    }
    
    printf("\n[ANALYZE] %s node:\n", is_root ? "ROOT" : "Internal");
    printf("  Left edge length: %.3le\n", node->left_len());
    printf("  Right edge length: %.3le\n", node->right_len());
    
    // Process children
    printf("  -> Processing left child...\n");
    NodeResult left = insert_inverters_bottom_up(node->left(), Rb, r, c, Co, T_constraint, false);
    
    printf("  -> Processing right child...\n");
    NodeResult right = insert_inverters_bottom_up(node->right(), Rb, r, c, Co, T_constraint, false);
    
    if (left.needs_inverter < 0 || right.needs_inverter < 0) {
        printf("  -> Child is infeasible, propagating failure\n");
        NodeResult result;
        result.max_delay = HUGE_VAL;
        result.total_cap = 0.0;
        result.min_stages = -1;
        result.needs_inverter = -1;
        return result;
    }
    
    double CL = left.total_cap;
    double CR = right.total_cap;
    double CA_left = c * node->left_len();
    double CA_right = c * node->right_len();
    double RA_left = r * node->left_len();
    double RA_right = r * node->right_len();
    
    printf("  Capacitances: CL=%.3le, CR=%.3le, CA_left=%.3le, CA_right=%.3le\n", 
           CL, CR, CA_left, CA_right);
    printf("  Child delays: left=%.3le, right=%.3le\n", left.max_delay, right.max_delay);
    printf("  Child stages: left=%d, right=%d\n", left.min_stages, right.min_stages);
    
    // Wire delay (no inverter at this node)
    double delay_left_wire = RA_left * (CL + CA_left/2.0) + left.max_delay;
    double delay_right_wire = RA_right * (CR + CA_right/2.0) + right.max_delay;
    double max_delay_wire = (delay_left_wire > delay_right_wire) ? 
                            delay_left_wire : delay_right_wire;
    
    printf("  Wire delay (no inverter): left=%.3le, right=%.3le, max=%.3le\n", 
           delay_left_wire, delay_right_wire, max_delay_wire);
    
    // Delay WITH inverter at this node
    double delay_left_inv = Rb * (CL + CA_left/2.0) + RA_left * CL + left.max_delay;
    double delay_right_inv = Rb * (CR + CA_right/2.0) + RA_right * CR + right.max_delay;
    double max_delay_inv = (delay_left_inv > delay_right_inv) ?
                           delay_left_inv : delay_right_inv;
    
    printf("  With inverter at node: left=%.3le, right=%.3le, max=%.3le\n",
           delay_left_inv, delay_right_inv, max_delay_inv);
    printf("  Constraint: %.3le\n", T_constraint);
    
    NodeResult result;
    result.total_cap = CL + CR + CA_left + CA_right;
    
    // Decision: do we NEED an inverter at this node?
    if (max_delay_inv > T_constraint || is_root) {
        printf("  -> %s\n", is_root ? "ROOT always has inverter" : 
               "CONSTRAINT VIOLATED - need inverter/segmentation");
        
        int left_segs = 1;
        int right_segs = 1;
        
        // Only calculate segments for branches that actually violate
        if (delay_left_inv > T_constraint) {
            printf("  -> LEFT exceeds constraint - calculating segments\n");
            left_segs = calculate_segments(node->left_len(), CL, left.max_delay,
                                          Rb, r, c, Co, T_constraint);
            if (left_segs < 0) {
                printf("  -> LEFT EDGE INFEASIBLE\n");
                result.max_delay = HUGE_VAL;
                result.min_stages = -1;
                result.needs_inverter = -1;
                return result;
            }
            printf("  -> Left needs %d segments\n", left_segs);
        } else {
            printf("  -> LEFT OK with 1 segment\n");
        }
        
        if (delay_right_inv > T_constraint) {
            printf("  -> RIGHT exceeds constraint - calculating segments\n");
            right_segs = calculate_segments(node->right_len(), CR, right.max_delay,
                                           Rb, r, c, Co, T_constraint);
            if (right_segs < 0) {
                printf("  -> RIGHT EDGE INFEASIBLE\n");
                result.max_delay = HUGE_VAL;
                result.min_stages = -1;
                result.needs_inverter = -1;
                return result;
            }
            printf("  -> Right needs %d segments\n", right_segs);
        } else {
            printf("  -> RIGHT OK with 1 segment\n");
        }
        
        printf("  -> Initial segmentation: left=%d, right=%d\n", left_segs, right_segs);
        
        // Stages: child stages + edge inverters + this node's inverter
        int left_stages = left.min_stages + left_segs;
        int right_stages = right.min_stages + right_segs;
        
        printf("  -> Stage count: left=%d, right=%d\n", left_stages, right_stages);
        
        // Fix parity
        if ((left_stages % 2) != (right_stages % 2)) {
            printf("  -> PARITY MISMATCH\n");
            if (left_stages < right_stages) {
                left_segs++;
                left_stages++;
                printf("     Adjusted: left_segs=%d stages=%d\n", left_segs, left_stages);
            } else {
                right_segs++;
                right_stages++;
                printf("     Adjusted: right_segs=%d stages=%d\n", right_segs, right_stages);
            }
        }
        
        // Root parity fix
        if (is_root && (left_stages % 2 != 0)) {
            printf("  -> ROOT has ODD stages - adding to both\n");
            left_segs++;
            right_segs++;
            left_stages++;
            right_stages++;
            printf("     Final: left_segs=%d stages=%d, right_segs=%d stages=%d\n",
                   left_segs, left_stages, right_segs, right_stages);
        }
        
        node->set_left_segments(left_segs);
        node->set_right_segments(right_segs);
        node->set_k(1);
        
        // Recalculate delay with final segmentation
        double seg_left_len = node->left_len() / left_segs;
        double seg_right_len = node->right_len() / right_segs;
        
        delay_left_inv = Rb * (CL + c * seg_left_len/2.0) + 
                        r * seg_left_len * CL + left.max_delay;
        delay_right_inv = Rb * (CR + c * seg_right_len/2.0) + 
                         r * seg_right_len * CR + right.max_delay;
        max_delay_inv = (delay_left_inv > delay_right_inv) ? 
                       delay_left_inv : delay_right_inv;
        
        printf("  -> Final delay: %.3le\n", max_delay_inv);
        
        result.max_delay = max_delay_inv;
        result.min_stages = (left_stages > right_stages) ? left_stages : right_stages;
        result.needs_inverter = 1;
        
    } else {
        printf("  -> Constraint satisfied - NO inverter needed at this node (k=0)\n");
        
        node->set_left_segments(1);
        node->set_right_segments(1);
        node->set_k(0);
        
        result.max_delay = max_delay_wire;
        result.min_stages = (left.min_stages > right.min_stages) ? 
                           left.min_stages : right.min_stages;
        result.needs_inverter = 0;
        
        printf("  -> Propagating: delay=%.3le, stages=%d\n", 
               result.max_delay, result.min_stages);
    }
    
    printf("\n");
    return result;
}


int calculate_segments(double edge_len, double C_down, double delay_down,
                      double Rb, double r, double c, double Co, double T_constraint) {
    printf("  [SEGMENT] Calculating segments (QUADRATIC SOLUTION):\n");
    
    if (edge_len <= 0) return 1;
    
    double T_available = T_constraint - delay_down;
    double driver_term = Rb * (Co + C_down);
    
    printf("  [SEGMENT]   T_available = %.3le - %.3le = %.3le\n", 
           T_constraint, delay_down, T_available);
    printf("  [SEGMENT]   Driver term = Rb*(Co + C_down) = %.3le\n", driver_term);
    
    if (T_available <= driver_term) {
        printf("  [SEGMENT]   INFEASIBLE: T_available <= driver term\n");
        return -1;
    }
    
    // Quadratic: (r*c/2)*L'^2 + (Rb*c + r*C_down)*L' - [T_available - driver_term] = 0
    double a = r * c / 2.0;
    double b = Rb * c + r * C_down;
    double c_const = -(T_available - driver_term);
    
    printf("  [SEGMENT]   Quadratic coefficients: a=%.3le, b=%.3le, c=%.3le\n", a, b, c_const);
    
    double discriminant = b*b - 4*a*c_const;
    printf("  [SEGMENT]   Discriminant = %.3le\n", discriminant);
    
    if (discriminant < 0) {
        printf("  [SEGMENT]   INFEASIBLE: negative discriminant\n");
        return -1;
    }
    
    // Take positive root
    double L_prime = (-b + sqrt(discriminant)) / (2*a);
    printf("  [SEGMENT]   Max segment length L' = %.3le\n", L_prime);
    
    if (L_prime <= 0) {
        printf("  [SEGMENT]   INFEASIBLE: L' <= 0\n");
        return -1;
    }
    
    int k = (int)ceil(edge_len / L_prime);
    printf("  [SEGMENT]   Segments needed k = ceil(%.3le / %.3le) = %d\n", 
           edge_len, L_prime, k);
    
    return k;
}




void build_tree_with_inverters(Node* node) {
    if (node->leaf()) {
        printf("  [BUILD] Leaf node %d - no modifications\n", node->label());
        return;
    }
    
    printf("[BUILD] Processing internal node:\n");
    printf("  Left edge: len=%.3le, segments=%d\n", 
           node->left_len(), node->left_segments());
    printf("  Right edge: len=%.3le, segments=%d\n", 
           node->right_len(), node->right_segments());
    
    // Process children first
    printf("  -> Recursing into left child...\n");
    build_tree_with_inverters(node->left());
    
    if (node->right()) {
        printf("  -> Recursing into right child...\n");
        build_tree_with_inverters(node->right());
    }
    
    // Insert inverter chains if needed
    int left_segs = node->left_segments();
    int right_segs = node->right_segments();
    
    // Insert chain on left edge
    if (left_segs > 1) {
        printf("  -> INSERTING %d inverters on left edge\n", left_segs - 1);
        double seg_len = node->left_len() / left_segs;
        printf("     Segment length: %.3le\n", seg_len);
        
        Node* chain_top = build_inverter_chain(node->left(), seg_len, left_segs - 1);
        
        printf("     Old left child: %p\n", (void*)node->left());
        node->set_left(chain_top);
        node->set_left_len(seg_len);
        printf("     New left child (chain top): %p\n", (void*)node->left());
    } else {
        printf("  -> No inverters needed on left edge\n");
    }
    
    // Insert chain on right edge
    if (right_segs > 1) {
        printf("  -> INSERTING %d inverters on right edge\n", right_segs - 1);
        double seg_len = node->right_len() / right_segs;
        printf("     Segment length: %.3le\n", seg_len);
        
        Node* chain_top = build_inverter_chain(node->right(), seg_len, right_segs - 1);
        
        printf("     Old right child: %p\n", (void*)node->right());
        node->set_right(chain_top);
        node->set_right_len(seg_len);
        printf("     New right child (chain top): %p\n", (void*)node->right());
    } else {
        printf("  -> No inverters needed on right edge\n");
    }
    
    printf("[BUILD] Done with this node\n\n");
}

Node* build_inverter_chain(Node* downstream, double seg_len, int num_inverters) {
    printf("     Building chain of %d inverters, seg_len=%.3le\n", 
           num_inverters, seg_len);
    
    Node* current = downstream;
    
    for (int i = 0; i < num_inverters; i++) {
        Node* inv = new Node(seg_len, -1.0, current, nullptr);
        inv->set_k(1);
        printf("       Inverter %d created at %p -> downstream %p\n", 
               i+1, (void*)inv, (void*)current);
        current = inv;
    }
    
    printf("     Chain complete, returning top: %p\n", (void*)current);
    return current;
}

void write_tree_with_inverters(Node* root, FILE* out, bool binary_mode) {
    if (!root) return;
    
    if (root->leaf()) {
        if (binary_mode) {
            int label = root->label();
            double cap = root->cap();
            fwrite(&label, sizeof(int), 1, out);
            fwrite(&cap, sizeof(double), 1, out);
        } else {
            // std::cout << "Leaf detected: " << std::endl
            //           << root->label() << " " <<  root->cap() << " " << std::endl;

            fprintf(out, "%d(%.10le)\n", root->label(), root->cap());
        }
    } else {
        // Post-order: children first
        if (root->left()) {
            write_tree_with_inverters(root->left(), out, binary_mode);
        }
        if (root->right() && root->right_len() >= 0) {
            write_tree_with_inverters(root->right(), out, binary_mode);
        }
        
        // Then write this node
        if (binary_mode) {
            int marker = -1;
            double left_len = root->left_len();
            double right_len = root->right_len();
            int k = root->k();
            fwrite(&marker, sizeof(int), 1, out);
            fwrite(&left_len, sizeof(double), 1, out);
            fwrite(&right_len, sizeof(double), 1, out);
            fwrite(&k, sizeof(int), 1, out);
        } else {
            // std::cout << "Non-leaf detected: " << std::endl
            //         << root->left_len() << " " <<  root->right_len() << " " << root->k() << std::endl
            //         << root->left() << " " << root->right() << std::endl;
            fprintf(out, "(%.10le %.10le %d)\n", 
                    root->left_len(), 
                    root->right_len(),
                    root->k());
        }
    }
}
