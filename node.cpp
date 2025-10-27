#include "node.h"


Node::Node(int lbl,double c) :
    isLeaf_(true),
    label_(lbl),
    cap_(c),
    leftLen_(0.0),
    rightLen_(0.0),
    left_(nullptr),
    right_(nullptr) ,

    c_prime_(0.0),
    c_downstream_(0.0),
    elmore_delay_(0.0),
    parent_(nullptr),
    parentLen_(0.0),
    k_(0),
    c_upstream_(0),
    t_upstream_(0),
    parity_(0)

    {}

Node::Node(double lLen, double rLen, Node* lChild, Node* rChild):
    isLeaf_(false),
    label_(0),
    cap_(0.0),
    leftLen_(lLen),
    rightLen_(rLen),
    left_(lChild),
    right_(rChild),
    
    c_prime_(0.0),
    c_downstream_(0.0),
    elmore_delay_(0.0),
    parent_(nullptr),
    parentLen_(0.0),
    k_(0),
    c_upstream_(0),
    t_upstream_(0),
    parity_(0) {}

Node::~Node() {
    if (!this->leaf()) {
        delete this->left();
        delete this->right();
    }
}
