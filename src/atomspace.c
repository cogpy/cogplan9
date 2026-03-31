/*
 * src/atomspace.c — AtomSpace portable C implementation
 */

#include <plan9cog/atomspace.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ── Internal id counter ────────────────────────────────────────── */
static unsigned long s_next_id = 1;

/* ── Truth value helpers ────────────────────────────────────────── */

truth_value_t *tv_create(double strength, double confidence)
{
    truth_value_t *tv = (truth_value_t *)malloc(sizeof(truth_value_t));
    if (!tv) return NULL;
    tv->strength   = strength;
    tv->confidence = confidence;
    return tv;
}

void tv_free(truth_value_t *tv)
{
    free(tv);
}

/* ── Internal atom allocator ────────────────────────────────────── */

static atom_t *atom_alloc(int type, const char *name)
{
    atom_t *a = (atom_t *)calloc(1, sizeof(atom_t));
    if (!a) return NULL;
    a->type = type;
    a->id   = s_next_id++;
    if (name && *name) {
        a->name = strdup(name);
        if (!a->name) { free(a); return NULL; }
    }
    return a;
}

/* ── Atom lifecycle ─────────────────────────────────────────────── */

atom_t *atom_create_node(const char *name)
{
    return atom_alloc(NODE, name ? name : "");
}

atom_t *atom_create_node_typed(const char *name, int type)
{
    return atom_alloc(type, name ? name : "");
}

atom_t *atom_create_link(const char *name, int count, atom_t **targets)
{
    atom_t *a = atom_alloc(LINK, name ? name : "");
    if (!a) return NULL;
    if (count > 0 && targets) {
        a->outgoing_set = (atom_t **)malloc((size_t)count * sizeof(atom_t *));
        if (!a->outgoing_set) { free(a->name); free(a); return NULL; }
        memcpy(a->outgoing_set, targets, (size_t)count * sizeof(atom_t *));
        a->outgoing_set_size = count;
    }
    return a;
}

atom_t *atom_create_link_typed(int type, int count, atom_t **targets)
{
    atom_t *a = atom_alloc(type, NULL);
    if (!a) return NULL;
    if (count > 0 && targets) {
        a->outgoing_set = (atom_t **)malloc((size_t)count * sizeof(atom_t *));
        if (!a->outgoing_set) { free(a); return NULL; }
        memcpy(a->outgoing_set, targets, (size_t)count * sizeof(atom_t *));
        a->outgoing_set_size = count;
    }
    return a;
}

void atom_free(atom_t *a)
{
    int i;
    if (!a) return;
    /* Recursively free outgoing set */
    for (i = 0; i < a->outgoing_set_size; i++)
        atom_free(a->outgoing_set[i]);
    free(a->outgoing_set);
    free(a->name);
    tv_free(a->tv);
    free(a);
}

void atom_free_shallow(atom_t *a)
{
    if (!a) return;
    free(a->outgoing_set);
    free(a->name);
    tv_free(a->tv);
    free(a);
}

/* ── AtomSpace lifecycle ─────────────────────────────────────────── */

#define ATOMSPACE_INITIAL_CAP 64

atomspace_t *atomspace_create(void)
{
    atomspace_t *as = (atomspace_t *)calloc(1, sizeof(atomspace_t));
    if (!as) return NULL;
    as->atoms = (atom_t **)calloc(ATOMSPACE_INITIAL_CAP, sizeof(atom_t *));
    if (!as->atoms) { free(as); return NULL; }
    as->capacity = ATOMSPACE_INITIAL_CAP;
    return as;
}

void atomspace_free(atomspace_t *as)
{
    int i;
    if (!as) return;
    for (i = 0; i < as->natoms; i++)
        atom_free(as->atoms[i]);
    free(as->atoms);
    free(as);
}

int atomspace_add(atomspace_t *as, atom_t *a)
{
    atom_t **tmp;
    if (!as || !a) return -1;
    if (as->natoms >= as->capacity) {
        int newcap = as->capacity * 2;
        tmp = (atom_t **)realloc(as->atoms,
                                  (size_t)newcap * sizeof(atom_t *));
        if (!tmp) return -1;
        as->atoms    = tmp;
        as->capacity = newcap;
    }
    as->atoms[as->natoms++] = a;
    return 0;
}

atom_t *atomspace_find(atomspace_t *as, unsigned long id)
{
    int i;
    if (!as) return NULL;
    for (i = 0; i < as->natoms; i++)
        if (as->atoms[i] && as->atoms[i]->id == id)
            return as->atoms[i];
    return NULL;
}

int atomspace_remove(atomspace_t *as, unsigned long id)
{
    int i;
    if (!as) return -1;
    for (i = 0; i < as->natoms; i++) {
        if (as->atoms[i] && as->atoms[i]->id == id) {
            atom_free(as->atoms[i]);
            as->atoms[i] = as->atoms[as->natoms - 1];
            as->natoms--;
            return 0;
        }
    }
    return -1;
}

atom_t **atomspace_query(atomspace_t *as, atom_predicate_t pred,
                         void *arg, int *count_out)
{
    int i, found = 0;
    atom_t **result;
    if (!as || !pred || !count_out) { if (count_out) *count_out = 0; return NULL; }
    /* First pass: count */
    for (i = 0; i < as->natoms; i++)
        if (as->atoms[i] && pred(as->atoms[i], arg)) found++;
    if (found == 0) { *count_out = 0; return NULL; }
    result = (atom_t **)malloc((size_t)found * sizeof(atom_t *));
    if (!result) { *count_out = 0; return NULL; }
    found = 0;
    for (i = 0; i < as->natoms; i++)
        if (as->atoms[i] && pred(as->atoms[i], arg))
            result[found++] = as->atoms[i];
    *count_out = found;
    return result;
}

int atomspace_size(const atomspace_t *as)
{
    if (!as) return 0;
    return as->natoms;
}
