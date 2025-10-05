#include "node.h"
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <stack>

typedef struct
{
    int label;
    double delay;
} elmore;

// tree.h
typedef struct {
    double max_delay;     // Maximum delay through this subtree
    double total_cap;     // Total capacitance (downstream)
    int needs_inverter;   // Whether inverter needed at this node
} NodeResult;

NodeResult insert_inverters_bottom_up(Node* node, double Rb, double r, double c, 
                                      double Co, double T_constraint);

Node * buildTree(FILE * in);
void preorder(Node * root, FILE * out);
void build_c_prime(Node * node, double Co, double r, double c,bool is_root);
double recur_downstream(Node * node);
void dp_downstream(Node * root);
void recur_delay(Node * node, double pDelay, double pResistance,double r,FILE* out);
void dp_delay (Node * root, double Rb,double re,FILE * out);
NodeResult insert_inverters_bottom_up(Node* node, double Rb, double r, double c, 
                                      double Co, double T_constraint);
int calculate_segments(double edge_len, double C_down, double delay_down,
                      double Rb, double r, double c, double T_constraint);
void build_tree_with_inverters(Node* node);
Node* build_inverter_chain(Node* downstream, double seg_len, int num_inverters);
void write_tree_with_inverters(Node* root, FILE* out, bool binary_mode);
