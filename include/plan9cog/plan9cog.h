/*
 * plan9cog/plan9cog.h — Top-level Plan9Cog portable C API
 *
 * Initialization, teardown, and high-level cognitive operations.
 */

#ifndef PLAN9COG_H
#define PLAN9COG_H

#include <plan9cog/atomspace.h>
#include <plan9cog/pln.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ── System lifecycle ───────────────────────────────────────────── */

/*
 * Initialize the global Plan9Cog instance.
 * Returns 0 on success, non-zero on failure.
 */
int plan9cog_init(void);

/*
 * Shut down the global Plan9Cog instance, freeing all resources.
 * Returns 0 on success, non-zero on failure.
 */
int plan9cog_shutdown(void);

/* ── Atomese execution ──────────────────────────────────────────── */

/*
 * Execute an Atomese expression string.
 * Returns a heap-allocated result string (caller must free()),
 * or NULL on error.
 *
 * Example:
 *   char *result = plan9cog_execute_atomese(
 *       "(Evaluation (PredicateNode \"foo\") (ListLink))");
 *   // result -> "(stv 1.0 1.0)"
 *   free(result);
 */
char *plan9cog_execute_atomese(const char *atomese);

/* ── Global AtomSpace access ─────────────────────────────────────── */
atomspace_t *plan9cog_get_atomspace(void);

/* ── Global PLN engine access ────────────────────────────────────── */
pln_engine_t *plan9cog_get_pln(void);

/* ── Cognitive statistics ────────────────────────────────────────── */
typedef struct cog_stats {
    unsigned long total_atoms;
    unsigned long total_inferences;
    unsigned long uptime_ms;
} cog_stats_t;

void plan9cog_get_stats(cog_stats_t *out);

/* ── Version ─────────────────────────────────────────────────────── */
const char *plan9cog_version(void);

#ifdef __cplusplus
}
#endif

#endif /* PLAN9COG_H */
