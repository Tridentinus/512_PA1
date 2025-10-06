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
    return root;
}

void preorder(Node * root, FILE *out) {
    if (!root) return;
        if (!root->leaf()) {
        fprintf(out, "(%.10le %.10le)\n",root->left_len(),root->right_len());
        preorder(root->left(),out);
        preorder(root->right(),out);
    } else{
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

NodeResult insert_inverters_bottom_up(Node* node, double Rb, double r, double c, double Co, double Cb, double T_constraint, bool is_root) {
    
  if (node->leaf()) {
        return {0.0, node->cap(), 0, true};
    }
    
    NodeResult left = insert_inverters_bottom_up(node->left(), Rb, r, c, Co, Cb, T_constraint, false);
    NodeResult right = insert_inverters_bottom_up(node->right(), Rb, r, c, Co, Cb, T_constraint, false);
    
    if (left.feasible == false || right.feasible == false) {
        return {HUGE_VAL, 0.0, -1, false};
    }
    
    double CL = left.total_cap;
    double CR = right.total_cap;
    double Cl_L = c * node->left_len();
    double Cl_R = c * node->right_len();
    double RA_left = r * node->left_len();
    double RA_right = r * node->right_len();

    double delay_left_pre = RA_left * (CL + Cl_L/2.0) + left.max_delay;
    double delay_right_pre = RA_right * (CR + Cl_R/2.0) + right.max_delay;
    double max_delay_pre = (delay_left_pre > delay_right_pre) ? delay_left_pre : delay_right_pre;

    double delay_left_post = Rb * (Co + CL + Cl_L) + RA_left * (Cl_L/2.0 + CL) + left.max_delay;
    double delay_right_post = Rb * (Co + CR + Cl_R) + RA_right * (Cl_R/2.0 + CR) + right.max_delay;
    double max_delay_post = (delay_left_post > delay_right_post) ? delay_left_post : delay_right_post;

    NodeResult result;
    result.total_cap = CL + CR + Cl_L + Cl_R;

    if (max_delay_post > T_constraint || is_root) {
        
      int left_segs = 1;
      int right_segs = 1;
      
      if (delay_left_post > T_constraint) {
        left_segs = calculate_segments(node->left_len(), CL, left.max_delay,Rb, r, c, Co, Cb, T_constraint);
        if (left_segs < 0) {
            result.max_delay = HUGE_VAL;
            result.min_stages = -1;
            result.feasible = false;
            return result;
        }
      }

      if (delay_right_post > T_constraint) {
        right_segs = calculate_segments(node->right_len(), CR, right.max_delay,Rb, r, c, Co, Cb, T_constraint);
        if (right_segs < 0) {
            result.max_delay = HUGE_VAL;
            result.min_stages = -1;
            result.feasible = false;
            return result;
        }
      } 
      
      int left_stages = left.min_stages + left_segs;
      int right_stages = right.min_stages + right_segs;
      if ((left_stages % 2) != (right_stages % 2)) {
        if (left_stages < right_stages) {
            left_segs++;
            left_stages++;
        } else {
            right_segs++;
            right_stages++;
        }
      }
      
      if (is_root && (left_stages % 2 != 0)) {
        left_segs++;
        right_segs++;
        left_stages++;
        right_stages++;
      }
      
      node->set_left_segments(left_segs);
      node->set_right_segments(right_segs);
      node->set_k(1);
      
      double seg_left_len = node->left_len() / left_segs;
      double seg_right_len = node->right_len() / right_segs;
      
      delay_left_post = Rb * (Co + CL + c * seg_left_len) + r * seg_left_len * (c * seg_left_len/2.0 + CL) + left.max_delay;

      delay_right_post = Rb * (Co + CR + c * seg_right_len) + r * seg_right_len * (c * seg_right_len/2.0 + CR) + right.max_delay;
      max_delay_post = (delay_left_post > delay_right_post) ? delay_left_post : delay_right_post;

      result.max_delay = max_delay_post;
      result.min_stages = (left_stages > right_stages) ? left_stages : right_stages;
      result.feasible = true;
    } else {
      node->set_left_segments(1);
      node->set_right_segments(1);
      node->set_k(0);
      
      result.max_delay = max_delay_pre;
      result.min_stages = (left.min_stages > right.min_stages) ? left.min_stages : right.min_stages;
      result.feasible = true;
    }
    
    return result;
}

int calculate_segments(double edge_len, double C_down, double delay_down,double Rb, double r, double c, double Co, double Cb, double T_constraint) {
  if (edge_len <= 0) return 1;
  
  double T_avail_first = T_constraint - delay_down;
  double driver_first = Rb * (Co + C_down);
  
  if (T_avail_first <= driver_first) {
      return -1;
  }
  
  double a1 = r * c / 2.0;
  double b1 = Rb * c + r * C_down;
  double c1 = -(T_avail_first - driver_first);
  double disc1 = b1*b1 - 4*a1*c1;
  
  if (disc1 < 0) return -1;
  
  double L_max_first = (-b1 + sqrt(disc1)) / (2*a1);
  int k_first = (int)ceil(edge_len / L_max_first);
  
  double T_avail_sub = T_constraint;  
  double driver_sub = Rb * (Co + Cb);

  if (T_avail_sub <= driver_sub) {
      return -1;
  }
  
  double a2 = r * c / 2.0;
  double b2 = Rb * c + r * Cb;
  double c2 = -(T_avail_sub - driver_sub);
  double disc2 = b2*b2 - 4*a2*c2;
  
  if (disc2 < 0) return -1;
  
  double L_max_sub = (-b2 + sqrt(disc2)) / (2*a2);
  int k_sub = (int)ceil(edge_len / L_max_sub);
  int k = (k_first > k_sub) ? k_first : k_sub;
  
  return k;
}

void build_tree_with_inverters(Node* node) {
    if (node->leaf()) {
      return;
    }
    build_tree_with_inverters(node->left());
    
    if (node->right()) {
      build_tree_with_inverters(node->right());
    }
    
    int left_segs = node->left_segments();
    int right_segs = node->right_segments();
    
    if (left_segs > 1) {
      double seg_len = node->left_len() / left_segs;        
      Node* chain_top = build_inverter_chain(node->left(), seg_len, left_segs - 1);
      
      node->set_left(chain_top);
      node->set_left_len(seg_len);
    }
    
    if (right_segs > 1) {
      double seg_len = node->right_len() / right_segs;        
      Node* chain_top = build_inverter_chain(node->right(), seg_len, right_segs - 1);
      node->set_right(chain_top);
      node->set_right_len(seg_len);
    } 

    return;
}

Node* build_inverter_chain(Node* downstream, double seg_len, int num_inverters) {    
  Node* current = downstream;
  for (int i = 0; i < num_inverters; i++) {
    Node* inv = new Node(seg_len, -1.0, current, nullptr);
    inv->set_k(1);
    current = inv;
  }
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
      fprintf(out, "%d(%.10le)\n", root->label(), root->cap());
    }
  } else {
      if (root->left()) {
          write_tree_with_inverters(root->left(), out, binary_mode);
      }
      if (root->right() && root->right_len() >= 0) {
          write_tree_with_inverters(root->right(), out, binary_mode);
      }
      
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
      fprintf(out, "(%.10le %.10le %d)\n", root->left_len(), root->right_len(),root->k());
    }
    }
}
