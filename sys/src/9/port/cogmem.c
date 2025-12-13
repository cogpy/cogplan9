/*
 * Cognitive Memory Management
 * 
 * Revolutionary memory management where memory allocation is driven
 * by attention values and cognitive importance, not just process priority.
 * 
 * Memory is treated as a cognitive resource allocated based on:
 *   - Short-term importance (STI) - immediate cognitive need
 *   - Long-term importance (LTI) - persistent cognitive value
 *   - Cognitive context - what the memory represents
 */

#include	"u.h"
#include	"../port/lib.h"
#include	"mem.h"
#include	"dat.h"
#include	"fns.h"
#include	"../port/error.h"

/* Cognitive Memory Block */
typedef struct CogMemBlock CogMemBlock;
struct CogMemBlock {
	void	*addr;		/* Memory address */
	ulong	size;		/* Block size */
	short	sti;		/* Importance for retention */
	short	lti;		/* Long-term value */
	int	type;		/* Memory type */
	ulong	lastaccess;	/* Last access time */
	CogMemBlock *next;	/* Next in list */
	Lock;
};

/* Memory types */
enum {
	MemAtom = 1,		/* AtomSpace atom */
	MemLink,		/* AtomSpace link */
	MemPattern,		/* Pattern memory */
	MemInference,		/* Inference result */
	MemAttention,		/* Attention allocation */
	MemGeneral,		/* General cognitive memory */
};

/* Cognitive Memory Pool */
typedef struct CogMemPool CogMemPool;
struct CogMemPool {
	CogMemBlock	*blocks;	/* Allocated blocks */
	int		nblocks;	/* Number of blocks */
	ulong		totalmem;	/* Total allocated */
	ulong		maxmem;		/* Maximum allowed */
	short		totalsti;	/* Total STI budget */
	short		totallti;	/* Total LTI budget */
	Lock;
};

static CogMemPool cogmempool;

/* Initialize cognitive memory system */
void
cogmeminit(void)
{
	cogmempool.blocks = nil;
	cogmempool.nblocks = 0;
	cogmempool.totalmem = 0;
	cogmempool.maxmem = 1024*1024*1024;	/* 1GB cognitive memory */
	cogmempool.totalsti = 10000;		/* Total STI budget */
	cogmempool.totallti = 5000;		/* Total LTI budget */
}

/* Allocate cognitive memory */
void*
cogalloc(ulong size, int type, short sti, short lti)
{
	CogMemBlock *block;
	void *addr;

	lock(&cogmempool);
	
	/* Check if we have memory budget */
	if(cogmempool.totalmem + size > cogmempool.maxmem) {
		/* Try to free low-importance memory */
		cogreclaim(size);
		if(cogmempool.totalmem + size > cogmempool.maxmem) {
			unlock(&cogmempool);
			return nil;
		}
	}

	/* Allocate memory */
	addr = malloc(size);
	if(addr == nil) {
		unlock(&cogmempool);
		return nil;
	}

	/* Create cognitive block */
	block = malloc(sizeof(CogMemBlock));
	if(block == nil) {
		free(addr);
		unlock(&cogmempool);
		return nil;
	}

	block->addr = addr;
	block->size = size;
	block->sti = sti;
	block->lti = lti;
	block->type = type;
	block->lastaccess = m->ticks;
	block->next = cogmempool.blocks;
	cogmempool.blocks = block;
	cogmempool.nblocks++;
	cogmempool.totalmem += size;

	unlock(&cogmempool);
	return addr;
}

/* Free cognitive memory */
void
cogfree(void *addr)
{
	CogMemBlock *block, *prev;

	if(addr == nil)
		return;

	lock(&cogmempool);
	
	prev = nil;
	for(block = cogmempool.blocks; block != nil; block = block->next) {
		if(block->addr == addr) {
			/* Remove from list */
			if(prev == nil)
				cogmempool.blocks = block->next;
			else
				prev->next = block->next;
			
			cogmempool.totalmem -= block->size;
			cogmempool.nblocks--;
			
			free(addr);
			free(block);
			unlock(&cogmempool);
			return;
		}
		prev = block;
	}
	
	unlock(&cogmempool);
}

/* Update memory importance */
void
cogmemupdate(void *addr, short sti, short lti)
{
	CogMemBlock *block;

	lock(&cogmempool);
	
	for(block = cogmempool.blocks; block != nil; block = block->next) {
		if(block->addr == addr) {
			lock(block);
			block->sti += sti;
			block->lti += lti;
			block->lastaccess = m->ticks;
			unlock(block);
			break;
		}
	}
	
	unlock(&cogmempool);
}

/* Reclaim low-importance memory */
int
cogreclaim(ulong needed)
{
	CogMemBlock *block, *victim, *prev, *vprev;
	int minscore, score;
	ulong reclaimed;

	reclaimed = 0;
	
	/* Find blocks with low importance */
	while(reclaimed < needed) {
		victim = nil;
		vprev = nil;
		minscore = 10000;
		
		prev = nil;
		for(block = cogmempool.blocks; block != nil; block = block->next) {
			/* Score based on STI, LTI, and access time */
			lock(block);
			score = block->sti + (block->lti / 10);
			/* Penalize old accesses */
			if(m->ticks - block->lastaccess > HZ*60)
				score /= 2;
			unlock(block);
			
			if(score < minscore) {
				minscore = score;
				victim = block;
				vprev = prev;
			}
			prev = block;
		}
		
		if(victim == nil)
			break;	/* Nothing to reclaim */
		
		/* Free the victim block */
		if(vprev == nil)
			cogmempool.blocks = victim->next;
		else
			vprev->next = victim->next;
		
		reclaimed += victim->size;
		cogmempool.totalmem -= victim->size;
		cogmempool.nblocks--;
		
		free(victim->addr);
		free(victim);
	}
	
	return reclaimed >= needed ? 0 : -1;
}

/* Decay memory importance over time */
void
cogmemdecay(void)
{
	CogMemBlock *block;

	lock(&cogmempool);
	
	for(block = cogmempool.blocks; block != nil; block = block->next) {
		lock(block);
		/* Decay STI faster than LTI */
		if(block->sti > 0)
			block->sti--;
		if(block->lti > 0 && m->ticks % (HZ*10) == 0)
			block->lti--;
		unlock(block);
	}
	
	unlock(&cogmempool);
}

/* Get memory statistics */
void
cogmemstats(ulong *total, ulong *max, int *nblocks)
{
	lock(&cogmempool);
	*total = cogmempool.totalmem;
	*max = cogmempool.maxmem;
	*nblocks = cogmempool.nblocks;
	unlock(&cogmempool);
}

/* Find memory by type */
CogMemBlock*
cogmemfind(int type)
{
	CogMemBlock *block;

	lock(&cogmempool);
	for(block = cogmempool.blocks; block != nil; block = block->next) {
		if(block->type == type) {
			unlock(&cogmempool);
			return block;
		}
	}
	unlock(&cogmempool);
	return nil;
}

/* Allocate atom memory */
void*
cogallocatom(ulong size, short sti)
{
	return cogalloc(size, MemAtom, sti, sti/2);
}

/* Allocate link memory */
void*
cogalloclink(ulong size, short sti)
{
	return cogalloc(size, MemLink, sti, sti/2);
}

/* Allocate pattern memory */
void*
cogallocpattern(ulong size)
{
	return cogalloc(size, MemPattern, 50, 100);
}

/* Allocate inference result memory */
void*
cogallocinfer(ulong size)
{
	return cogalloc(size, MemInference, 100, 20);
}

/* Promote memory importance */
void
cogmempromote(void *addr)
{
	cogmemupdate(addr, 10, 5);
}

/* Demote memory importance */
void
cogmemdemote(void *addr)
{
	cogmemupdate(addr, -10, -5);
}

/* Touch memory to update access time */
void
cogmemtouch(void *addr)
{
	CogMemBlock *block;

	lock(&cogmempool);
	for(block = cogmempool.blocks; block != nil; block = block->next) {
		if(block->addr == addr) {
			lock(block);
			block->lastaccess = m->ticks;
			unlock(block);
			break;
		}
	}
	unlock(&cogmempool);
}

/* Cognitive garbage collection */
int
coggc(void)
{
	CogMemBlock *block, *next, *prev;
	int collected;
	ulong now;

	collected = 0;
	now = m->ticks;

	lock(&cogmempool);
	
	prev = nil;
	block = cogmempool.blocks;
	while(block != nil) {
		next = block->next;
		
		lock(block);
		/* Collect if importance is zero and not accessed recently */
		if(block->sti <= 0 && block->lti <= 0 && 
		   now - block->lastaccess > HZ*60) {
			unlock(block);
			
			/* Remove from list */
			if(prev == nil)
				cogmempool.blocks = next;
			else
				prev->next = next;
			
			cogmempool.totalmem -= block->size;
			cogmempool.nblocks--;
			free(block->addr);
			free(block);
			collected++;
			
			block = next;
			continue;
		}
		unlock(block);
		
		prev = block;
		block = next;
	}
	
	unlock(&cogmempool);
	return collected;
}
