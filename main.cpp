// main.cpp
#include "tree.h"

int main(int argc, char * argv[]) {
    // Expected:
    // argv[1] = time constraint (double)
    // argv[2] = inverter param file
    // argv[3] = wire param file
    // argv[4] = input RC tree (post-order text)
    // argv[5] = out1: pre-order text
    // argv[6] = out2: elmore (binary)
    // argv[7] = out3: ttopo (text)  -- will be empty for now
    // argv[8] = out4: btopo (binary) -- will be empty for now
    if (argc < 6) return EXIT_FAILURE;

    // const double T_max = atof(argv[1]);

    FILE * invIn  = fopen(argv[2], "r");
    FILE * wireIn = fopen(argv[3], "r");
    FILE * topIn  = fopen(argv[4], "r");
    if (!invIn || !wireIn || !topIn) {
        if (invIn)  fclose(invIn);
        if (wireIn) fclose(wireIn);
        if (topIn)  fclose(topIn);
        return EXIT_FAILURE;
    }

    Node * root = buildTree(topIn);
    fclose(topIn);
    if (!root) {
        fclose(invIn);
        fclose(wireIn);
        return EXIT_FAILURE;
    }

    FILE * preOut    = fopen(argv[5], "w");   
    FILE * elmoreOut = fopen(argv[6], "wb");  
    FILE * ttopoOut  = fopen(argv[7], "w");   
    FILE * btopoOut  = fopen(argv[8], "wb"); 

    if (!preOut || !elmoreOut || !ttopoOut || !btopoOut) {
        if (preOut)    fclose(preOut);
        if (elmoreOut) fclose(elmoreOut);
        if (ttopoOut)  fclose(ttopoOut);
        if (btopoOut)  fclose(btopoOut);
        delete root;
        fclose(invIn);
        fclose(wireIn);
        return EXIT_FAILURE;
    }
    preorder(root, preOut);
    // fclose(preOut);
    char buf[256];
    double C_in = 0.0, C_out = 0.0, R_inv = 0.0;
    if (fgets(buf, sizeof(buf), invIn)) {
        sscanf(buf, "%le %le %le\n", &C_in, &C_out, &R_inv);
    }
    double r = 0.0, c = 0.0;
    if (fgets(buf, sizeof(buf), wireIn)) {
        sscanf(buf, "%le %le\n", &r, &c);
    }
    fclose(invIn);
    fclose(wireIn);
    build_c_prime_dp(root, C_out, c, /*is_root=*/true);
    dp_downstream(root);
    dp_delay(root, R_inv, r, elmoreOut);
    fclose(elmoreOut);

    
    double Tb = R_inv * C_out;

    inverter_insertion(root,atof(argv[1]), R_inv, r, c, C_out, C_in, Tb);
    
    postorder_with_inverters(root,ttopoOut);
    fclose(ttopoOut);
    postorder_with_inverters_binary(root,btopoOut);
    fclose(btopoOut);
    std::cout << "Pre-order output written to " << argv[5] << std::endl;
    preorder(root, preOut);
    delete root;
    return EXIT_SUCCESS;
}
