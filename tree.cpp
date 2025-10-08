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

