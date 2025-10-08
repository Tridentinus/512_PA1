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
