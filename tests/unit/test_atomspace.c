#include "../test_macros.h"
#include <plan9cog/atomspace.h>

void test_atom_creation() {
    TEST_CASE("Atom Creation");
    atom_t* node = atom_create_node("test_node");
    ASSERT(node != NULL);
    ASSERT(node->type == NODE);
    ASSERT_STR_EQ(node->name, "test_node");
    atom_free(node);
}

void test_link_creation() {
    TEST_CASE("Link Creation");
    atom_t* target1 = atom_create_node("target1");
    atom_t* target2 = atom_create_node("target2");
    atom_t* targets[] = {target1, target2};
    atom_t* link = atom_create_link("test_link", 2, targets);
    ASSERT(link != NULL);
    ASSERT(link->type == LINK);
    ASSERT_STR_EQ(link->name, "test_link");
    ASSERT(link->outgoing_set_size == 2);
    ASSERT(link->outgoing_set[0] == target1);
    ASSERT(link->outgoing_set[1] == target2);
    atom_free(link);
    // atom_free(target1); // Freed by atom_free(link)
    // atom_free(target2); // Freed by atom_free(link)
}

int main() {
    test_atom_creation();
    test_link_creation();
    return TEST_SUMMARY();
}
