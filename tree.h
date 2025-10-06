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

// tree.h
typedef struct {
    double max_delay;     
    double total_cap;     
    int min_stages;      
    bool feasible;   
} NodeResult;
struct ParityCheckResult {
    bool all_even;
    int min_stages;
    int max_stages;
    bool all_same;
};
ParityCheckResult validate_leaf_parity(Node* root);
void check_leaf_stages_helper(Node* node, int current_stages, 
                              std::vector<std::pair<int,int>>& leaf_stages);
Node * buildTree(FILE * in);
void preorder(Node * root, FILE * out);
void build_c_prime(Node * node, double Co, double c,bool is_root= false);
void build_c_prime_dp(Node * root, double Co, double c, bool is_root);
double recur_downstream(Node * node);
void dp_downstream(Node * root);
void recur_delay(Node * node, double pDelay, double pResistance,double r,FILE* out);
void dp_delay (Node * root, double Rb,double re,FILE * out);
// tree.h - add is_root parameter
NodeResult insert_inverters_bottom_up(Node* node, double Rb, double r, double c, 
                                      double Co, double Cb, double T_constraint, bool is_root);
int calculate_segments(double edge_len, double C_down, double delay_down,
                      double Rb, double r, double c, double Co, double Cb,
                      double T_constraint);
void build_tree_with_inverters(Node* node);
Node* build_inverter_chain(Node* downstream, double seg_len, int num_inverters);
void write_tree_with_inverters(Node* root, FILE* out, bool binary_mode);
