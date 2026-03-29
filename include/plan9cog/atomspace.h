/*
 * plan9cog/atomspace.h — Portable C AtomSpace API for cogplan9
 *
 * Provides atom creation, hypergraph links, truth values, and
 * attention values for use in unit and integration tests.
 */

#ifndef PLAN9COG_ATOMSPACE_H
#define PLAN9COG_ATOMSPACE_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── Atom types ─────────────────────────────────────────────────── */
typedef enum atom_type {
    NODE            = 1,
    LINK            = 2,
    CONCEPT_NODE    = 3,
    PREDICATE_NODE  = 4,
    EVALUATION_LINK = 5,
    INHERITANCE_LINK= 6,
    SIMILARITY_LINK = 7,
    IMPLICATION_LINK= 8,
    LIST_LINK       = 9,
    EXECUTION_LINK  = 10
} atom_type_t;

/* ── Truth value ────────────────────────────────────────────────── */
typedef struct truth_value {
    double strength;    /* probability [0.0, 1.0] */
    double confidence;  /* confidence  [0.0, 1.0] */
} truth_value_t;

/* ── Attention value (ECAN) ─────────────────────────────────────── */
typedef struct attention_value {
    short sti;   /* short-term importance  */
    short lti;   /* long-term importance   */
    short vlti;  /* very-long-term importance */
} attention_value_t;

/* ── Atom ───────────────────────────────────────────────────────── */
typedef struct atom {
    int               type;
    char             *name;
    struct atom     **outgoing_set;
    int               outgoing_set_size;
    truth_value_t    *tv;
    attention_value_t av;
    unsigned long     id;
} atom_t;

/* ── AtomSpace ──────────────────────────────────────────────────── */
typedef struct atomspace {
    atom_t **atoms;
    int      natoms;
    int      capacity;
} atomspace_t;

/* ── Atom lifecycle ─────────────────────────────────────────────── */

/* Create a plain node with type NODE */
atom_t *atom_create_node(const char *name);

/* Create a node with an explicit type (e.g. CONCEPT_NODE) */
atom_t *atom_create_node_typed(const char *name, int type);

/* Create a link with type LINK */
atom_t *atom_create_link(const char *name, int count, atom_t **targets);

/* Create a link with an explicit type (e.g. IMPLICATION_LINK) */
atom_t *atom_create_link_typed(int type, int count, atom_t **targets);

/* Deep-free an atom and its outgoing set (recursively) */
void atom_free(atom_t *a);

/* Shallow-free a single atom without touching its outgoing set */
void atom_free_shallow(atom_t *a);

/* ── AtomSpace lifecycle ─────────────────────────────────────────── */
atomspace_t *atomspace_create(void);
void         atomspace_free(atomspace_t *as);

/* Add atom to space (does not transfer ownership of outgoing atoms) */
int  atomspace_add(atomspace_t *as, atom_t *a);

/* Find atom by id; returns NULL if not found */
atom_t *atomspace_find(atomspace_t *as, unsigned long id);

/* Remove atom by id; returns 0 on success, -1 if not found */
int atomspace_remove(atomspace_t *as, unsigned long id);

/* Query: call pred(atom, arg) for each atom; returns matching atoms */
typedef int (*atom_predicate_t)(const atom_t *a, void *arg);
atom_t **atomspace_query(atomspace_t *as, atom_predicate_t pred,
                         void *arg, int *count_out);

/* Return number of atoms in space */
int atomspace_size(const atomspace_t *as);

/* ── Truth value helpers ────────────────────────────────────────── */
truth_value_t *tv_create(double strength, double confidence);
void           tv_free(truth_value_t *tv);

#ifdef __cplusplus
}
#endif

#endif /* PLAN9COG_ATOMSPACE_H */
