#include "../test_macros.h"
#include <plan9cog/pln.h>

void test_pln_truth_value() {
    TEST_CASE("PLN Truth Value");
    truth_value_t* tv = pln_tv_create(0.8, 0.1);
    ASSERT(tv != NULL);
    ASSERT(tv->strength == 0.8);
    ASSERT(tv->confidence == 0.1);
    pln_tv_free(tv);
}

void test_pln_forward_inference() {
    TEST_CASE("PLN Forward Inference");
    atom_t* a = atom_create_node("A");
    atom_t* b = atom_create_node("B");
    atom_t* implication_targets[] = {a, b};
    atom_t* implication = atom_create_link("Implication", 2, implication_targets);

    truth_value_t* tv_a = pln_tv_create(0.9, 0.9);
    truth_value_t* tv_imp = pln_tv_create(0.7, 0.9);
    a->tv = tv_a;
    implication->tv = tv_imp;

    truth_value_t* tv_b = pln_forward_inference(implication, a);
    ASSERT(tv_b != NULL);
    // Simplified calculation for demonstration
    ASSERT(tv_b->strength > 0.6 && tv_b->strength < 0.7);
    ASSERT(tv_b->confidence > 0.8 && tv_b->confidence < 0.9);

    pln_tv_free(tv_b);
    atom_free(implication);

}

int main() {
    test_pln_truth_value();
    test_pln_forward_inference();
    return TEST_SUMMARY();
}
