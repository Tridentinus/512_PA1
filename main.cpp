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
    if (argc != 9) return EXIT_FAILURE;

    // ---- Parse time constraint (not used yet; needed for later stages) ----
    const double T_max = atof(argv[1]);

    // ---- Open inputs (topology, inverter, wire) ----
    FILE * invIn  = fopen(argv[2], "r");
    FILE * wireIn = fopen(argv[3], "r");
    FILE * topIn  = fopen(argv[4], "r");
    if (!invIn || !wireIn || !topIn) {
        if (invIn)  fclose(invIn);
        if (wireIn) fclose(wireIn);
        if (topIn)  fclose(topIn);
        return EXIT_FAILURE;
    }

    // ---- Build tree from post-order topology (input #3) ----
    Node * root = buildTree(topIn);
    fclose(topIn);
    if (!root) {
        fclose(invIn);
        fclose(wireIn);
        return EXIT_FAILURE;
    }

    // ---- Open outputs ----
    FILE * preOut    = fopen(argv[5], "w");   // pre-order text
    FILE * elmoreOut = fopen(argv[6], "wb");  // elmore binary
    FILE * ttopoOut  = fopen(argv[7], "w");   // text topology with inverters (empty for now)
    FILE * btopoOut  = fopen(argv[8], "wb");  // binary topology (empty for now)

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

    // =========================
    // Output 1: pre-order text
    // =========================
    std::cout << "Pre-order traversal of the input tree:\n";
    preorder(root, preOut);
    fclose(preOut);
    std::cout << "****************************\n";

    // =========================
    // Prepare parameters (inv, wire) for Output 2
    // =========================
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
    // Print parameters
    std::cout << "Inverter parameters:\n";
    std::cout << "  C_in  = " << C_in << "\n";
    std::cout << "  C_out = " << C_out << "\n";
    std::cout << "  R_inv = " << R_inv << "\n";
    std::cout << "Wire parameters:\n";
    std::cout << "  r = " << r << "\n";
    std::cout << "  c = " << c << "\n";
    std::cout << "****************************\n";
    // =========================
    // Output 2: Elmore delays (binary)
    // =========================
    // Build c' (local/lumped caps), then c_downstream, then t_j
    std::cout << "Calculating Elmore delays...\n";

    std::cout << "  Building c'...\n";
    build_c_prime(root, C_out, r, c, /*is_root=*/true);
    std::cout << "****************************\n";
    std::cout << "  Calculating downstream capacitances...\n";
    dp_downstream(root);
    std::cout << "****************************\n";
    std::cout << "  Calculating Elmore delays...\n";
    dp_delay(root, R_inv, r, elmoreOut);
    std::cout << "****************************\n";
    fclose(elmoreOut);
    printf("\n========== INVERTER INSERTION ==========\n");
    NodeResult result = insert_inverters_bottom_up(root, R_inv, r, c, C_out, T_max,true);

    if (result.needs_inverter < 0) {
        printf("\n========== INFEASIBLE ==========\n");
        fprintf(stderr, "No feasible solution for time constraint %.3le\n", T_max);
        // Create empty files
        FILE* f3 = fopen(argv[7], "w");
        if (f3) fclose(f3);
        FILE* f4 = fopen(argv[8], "wb");
        if (f4) fclose(f4);
        delete root;
        return EXIT_SUCCESS;
    }

    printf("\n========== ROOT ANALYSIS ==========\n");
    printf("Root stages: %d\n", result.min_stages);
    printf("Root max delay: %.3le\n", result.max_delay);



    printf("\n========== BUILDING TREE WITH INVERTERS ==========\n");
    build_tree_with_inverters(root);


    std::cout << "Post-order traversal of the modified tree:\n";
    write_tree_with_inverters(root, ttopoOut, /*binary_mode=*/false);
    std::cout << "****************************\n";
    // Pre-order with inverters
    std::cout << "Pre-order traversal of the modified tree:\n";
    preorder(root, stdout);
    std::cout << "****************************\n";
    fclose(ttopoOut);

    delete root;
    return EXIT_SUCCESS;
}
