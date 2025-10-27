#include "node.h"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <stack>
#include <vector>

typedef struct
{
    int label;
    double delay;
} elmore;


Node * buildTree(FILE * in);
void preorder(Node * root, FILE * out);
void build_c_prime(Node * node, double Co, double c,bool is_root= false);
void build_c_prime_dp(Node * root, double Co, double c, bool is_root);
double recur_downstream(Node * node);
void dp_downstream(Node * root);
void recur_delay(Node * node, double pDelay, double pResistance,double r,FILE* out);
void dp_delay (Node * root, double Rb,double re,FILE * out);


double compute_max_distance_hyp(double CT, double Tmax, double T_constraint, 
                                double Rb, double r, double c, double Co);

Node* insert_inverter(Node* node, double L_max, double Rb, double r, double c, 
                      double Co, double Cb, double Tb);

std::pair<Node*, Node*> insert_repeater(Node* node, double L_max, 
                                        double Rb, double r, double c, 
                                        double Co, double Cb, double Tb);

Node* insert_inverter_on_child(Node* parent,int child_side,
                                double Rb, double r, double c, 
                                double Co, double Cb, double Tb);

void process_node(Node* node, double T_constraint, 
                  double Rb, double r, double c, double Co, double Cb, double Tb);

void inverter_insertion(Node* root, double T_constraint, 
                        double Rb, double r, double c, double Co, double Cb, double Tb);

// Output functions for topology with inverters
void postorder_with_inverters(Node* root, FILE* out);
void postorder_with_inverters_binary(Node* root, FILE* out);
