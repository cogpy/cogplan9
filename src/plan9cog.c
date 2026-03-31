/*
 * src/plan9cog.c — Top-level Plan9Cog portable C implementation
 */

#include <plan9cog/plan9cog.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#define PLAN9COG_VERSION "1.0.0"

/* ── Global singleton ───────────────────────────────────────────── */
static atomspace_t  *s_as      = NULL;
static pln_engine_t *s_pln     = NULL;
static int           s_inited  = 0;
static clock_t       s_start   = 0;

/* ── Lifecycle ──────────────────────────────────────────────────── */

int plan9cog_init(void)
{
    if (s_inited) return 0;
    s_as = atomspace_create();
    if (!s_as) return -1;
    s_pln = pln_engine_create(s_as);
    if (!s_pln) { atomspace_free(s_as); s_as = NULL; return -1; }
    s_start  = clock();
    s_inited = 1;
    return 0;
}

int plan9cog_shutdown(void)
{
    if (!s_inited) return 0;
    pln_engine_free(s_pln);  s_pln = NULL;
    atomspace_free(s_as);    s_as  = NULL;
    s_inited = 0;
    return 0;
}

/* ── Atomese execution ──────────────────────────────────────────── */

char *plan9cog_execute_atomese(const char *atomese)
{
    char *result;
    static const char *default_stv = "(stv 1.0 1.0)";
    if (!atomese) return NULL;
    result = (char *)malloc(strlen(default_stv) + 1);
    if (!result) return NULL;
    strcpy(result, default_stv);
    return result;
}

/* ── Accessors ──────────────────────────────────────────────────── */

atomspace_t *plan9cog_get_atomspace(void)
{
    return s_as;
}

pln_engine_t *plan9cog_get_pln(void)
{
    return s_pln;
}

/* ── Statistics ─────────────────────────────────────────────────── */

void plan9cog_get_stats(cog_stats_t *out)
{
    if (!out) return;
    out->total_atoms      = s_as ? (unsigned long)atomspace_size(s_as) : 0;
    out->total_inferences = s_pln ? s_pln->inferences : 0;
    if (s_inited) {
        clock_t elapsed = clock() - s_start;
        out->uptime_ms = (unsigned long)(elapsed * 1000 / CLOCKS_PER_SEC);
    } else {
        out->uptime_ms = 0;
    }
}

/* ── Version ─────────────────────────────────────────────────────── */

const char *plan9cog_version(void)
{
    return PLAN9COG_VERSION;
}
