#ifndef NODE_H
#define NODE_H
#include <cstdio>

class Node {
private:
    bool isLeaf_;
    // leaf properties
    int label_;
    double cap_;
    // non-leaf properties
    double leftLen_;
    double rightLen_;
    Node * left_;
    Node * right_;

    // RC properties
    double c_prime_;

    // Elmore Properties
    double c_downstream_;
    double elmore_delay_;

    // Inverter insertion properties
    int k_;

public:
    Node(int lbl,double c);
    Node(double lLen, double rLen, Node* lChild, Node* rChild);
    
    ~Node();

    bool leaf() const{return isLeaf_;};
    
    int label() const{return label_;};
    double cap() const{return cap_;};
    
    double left_len() const{return leftLen_;};
    double right_len() const{return rightLen_;};

    Node * left() const{return left_;};
    bool has_left(const Node* n);
    void set_left(Node* n) {left_ = n;};

    Node * right() const{return right_;};
    bool has_right(const Node* n);
    void set_right(Node* n) {right_ = n;};

    void set_left_len(double v) {leftLen_ = v;};
    void set_right_len(double v) {rightLen_ = v;};

    
    double c_prime() const{return c_prime_;};
    void set_c_prime(double v) {c_prime_ =v;};
    void add_c_prime(double dv) {c_prime_ +=dv;};

    double c_downstream() const{return c_downstream_;};
    void set_c_downstream(double v) {c_downstream_=v;};

    double elmore_delay() const{return elmore_delay_;};
    void set_elmore_delay(double v) {elmore_delay_=v;};

    int k() const{return k_;};
    void set_k(int v) {k_ = v;};
    

};
#endif
