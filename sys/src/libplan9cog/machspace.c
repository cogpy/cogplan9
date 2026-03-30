/*
 * MachSpace - distributed hypergraph memory system for Plan9Cog
 *
 * Leverages Plan 9's network transparency via 9P protocol.
 * Remote cognitive services appear as local file systems.
 *
 * Usage:
 *   machspace9pconnect(ms, "tcp!remotehost!564")
 *   -> Mounts remote cogfs at /n/cog/remotehost
 *   -> Atoms on remote machine are transparently accessible
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <plan9cog.h>

/* Remote node tracking */
typedef struct RemoteNode RemoteNode;
struct RemoteNode {
	char		*addr;		/* Network address */
	char		*mountpt;	/* Mount point */
	int		fd;		/* Connection fd */
	AtomSpace	*cache;		/* Cached atoms from remote */
	vlong		lastSync;	/* Last sync time */
	int		connected;	/* Connection status */
};

/* Extended MachSpace with 9P support */
struct MachSpace9P {
	MachSpace	base;
	RemoteNode	**nodes;
	int		nnodes;
	int		maxnodes;
	char		*basemount;	/* Base mount point: /n/cog */
	Lock;
};

static char *basemountpt = "/n/cog";

MachSpace*
machspaceinit(AtomSpace *local)
{
	MachSpace *ms;

	ms = mallocz(sizeof(MachSpace), 1);
	if(ms == nil)
		return nil;

	ms->local = local;
	ms->remote = nil;
	ms->nremote = 0;
	ms->hosts = nil;

	return ms;
}

void
machspacefree(MachSpace *ms)
{
	int i;

	if(ms == nil)
		return;

	lock(ms);

	for(i = 0; i < ms->nremote; i++){
		free(ms->hosts[i]);
	}
	free(ms->hosts);
	free(ms->remote);

	unlock(ms);
	free(ms);
}

/*
 * Connect to remote cognitive service via 9P
 *
 * addr format: "tcp!hostname!port" or "net!address"
 * Creates mount point at /n/cog/<hostname>
 *
 * Plan 9 style: remote atoms accessible as files:
 *   /n/cog/hostname/atoms
 *   /n/cog/hostname/atomctl
 *   /n/cog/hostname/stats
 */
int
machspace9pconnect(MachSpace *ms, char *addr)
{
	char **newhosts;
	AtomSpace **newremote;
	int fd;
	char mountpt[256];
	char *host;
	char *p;

	if(ms == nil || addr == nil)
		return -1;

	/* Extract hostname from addr (tcp!host!port -> host) */
	host = strdup(addr);
	p = strchr(host, '!');
	if(p != nil){
		p++;
		char *end = strchr(p, '!');
		if(end != nil)
			*end = '\0';
		memmove(host, p, strlen(p) + 1);
	}

	/* Create mount point path */
	snprint(mountpt, sizeof(mountpt), "%s/%s", basemountpt, host);

	/* Ensure base mount directory exists */
	if(access(basemountpt, AEXIST) < 0){
		/* Create directory - in Plan 9 this would be via mntgen */
		fd = create(basemountpt, OREAD, DMDIR|0755);
		if(fd >= 0)
			close(fd);
	}

	/* Dial the remote service */
	fd = dial(addr, nil, nil, nil);
	if(fd < 0){
		free(host);
		werrstr("dial %s: %r", addr);
		return -1;
	}

	/* Mount the remote cogfs
	 * This is the Plan 9 magic - network becomes local namespace */
	if(mount(fd, -1, mountpt, MREPL|MCREATE, "") < 0){
		close(fd);
		free(host);
		werrstr("mount %s at %s: %r", addr, mountpt);
		return -1;
	}

	/* Add to remote list */
	lock(ms);

	newhosts = realloc(ms->hosts, sizeof(char*) * (ms->nremote + 1));
	newremote = realloc(ms->remote, sizeof(AtomSpace*) * (ms->nremote + 1));

	if(newhosts == nil || newremote == nil){
		unlock(ms);
		unmount(nil, mountpt);
		close(fd);
		free(host);
		return -1;
	}

	ms->hosts = newhosts;
	ms->remote = newremote;

	ms->hosts[ms->nremote] = strdup(mountpt);
	ms->remote[ms->nremote] = atomspacecreate(); /* Local cache */
	ms->nremote++;

	unlock(ms);
	free(host);

	return 0;
}

/* Legacy connect - stores host info but doesn't establish 9P */
int
machspaceconnect(MachSpace *ms, char *host)
{
	/* Try 9P connection first */
	char addr[256];
	snprint(addr, sizeof(addr), "tcp!%s!564", host);

	if(machspace9pconnect(ms, addr) == 0)
		return 0;

	/* Fall back to storing host for later */
	char **newhosts;
	AtomSpace **newremote;

	if(ms == nil || host == nil)
		return -1;

	lock(ms);

	newhosts = realloc(ms->hosts, sizeof(char*) * (ms->nremote + 1));
	newremote = realloc(ms->remote, sizeof(AtomSpace*) * (ms->nremote + 1));

	if(newhosts == nil || newremote == nil){
		unlock(ms);
		return -1;
	}

	ms->hosts = newhosts;
	ms->remote = newremote;

	ms->hosts[ms->nremote] = strdup(host);
	ms->remote[ms->nremote] = nil;
	ms->nremote++;

	unlock(ms);
	return 0;
}

/*
 * Find atom by ID across network
 * Uses Plan 9 file I/O to query remote cogfs
 */
static Atom*
machspace9pfindremote(char *mountpt, ulong id)
{
	char path[512];
	char buf[8192];
	int fd, n;
	Atom *a;
	char *fields[16];
	int nfields;
	TruthValue tv;
	AttentionValue av;

	/* Read from remote atomctl: echo "find <id>" | rc */
	snprint(path, sizeof(path), "%s/atoms", mountpt);

	fd = open(path, OREAD);
	if(fd < 0)
		return nil;

	/* Search for atom ID in atoms file */
	while((n = read(fd, buf, sizeof(buf)-1)) > 0){
		buf[n] = '\0';

		/* Parse lines looking for our ID */
		char *line = buf;
		while(line != nil && *line != '\0'){
			char *next = strchr(line, '\n');
			if(next != nil)
				*next++ = '\0';

			nfields = tokenize(line, fields, nelem(fields));
			if(nfields >= 2){
				ulong foundid = strtoul(fields[0], nil, 10);
				if(foundid == id){
					/* Found it - parse and create local copy */
					/* Format: id type name strength confidence */
					close(fd);
					return nil; /* Would create atom here */
				}
			}
			line = next;
		}
	}

	close(fd);
	return nil;
}

Atom*
machspacefind(MachSpace *ms, ulong id)
{
	Atom *a;
	int i;

	if(ms == nil)
		return nil;

	/* Search local first */
	a = atomfind(ms->local, id);
	if(a != nil)
		return a;

	/* Search remote spaces via 9P */
	lock(ms);
	for(i = 0; i < ms->nremote; i++){
		/* Check local cache first */
		if(ms->remote[i] != nil){
			a = atomfind(ms->remote[i], id);
			if(a != nil){
				unlock(ms);
				return a;
			}
		}

		/* Query remote via 9P file access */
		if(ms->hosts[i] != nil){
			a = machspace9pfindremote(ms->hosts[i], id);
			if(a != nil){
				unlock(ms);
				return a;
			}
		}
	}
	unlock(ms);

	return nil;
}

/* Internal: copy atom from source to destination atomspace */
static Atom*
copyatom(AtomSpace *dst, Atom *src)
{
	Atom *copy;
	Atom **outgoing;
	int i;
	TruthValue tv;
	AttentionValue av;

	if(src == nil)
		return nil;

	if(src->noutgoing > 0 && src->outgoing != nil){
		/* Copy outgoing atoms first */
		outgoing = mallocz(sizeof(Atom*) * src->noutgoing, 1);
		if(outgoing == nil)
			return nil;

		for(i = 0; i < src->noutgoing; i++){
			/* For links, we need to find or copy the outgoing atoms */
			outgoing[i] = atomfind(dst, src->outgoing[i]->id);
			if(outgoing[i] == nil){
				outgoing[i] = copyatom(dst, src->outgoing[i]);
			}
		}

		copy = linkcreate(dst, src->type, outgoing, src->noutgoing);
		free(outgoing);
	} else {
		copy = atomcreate(dst, src->type, src->name);
	}

	if(copy == nil)
		return nil;

	/* Copy truth and attention values */
	tv = atomgettruth(src);
	atomsettruth(copy, tv);
	av = atomgetattention(src);
	atomsetattention(copy, av);

	return copy;
}

/* Internal: merge atom into destination, updating truth values */
static void
mergeatom(Atom *dst, Atom *src)
{
	TruthValue tvdst, tvsrc, tvmerged;

	if(dst == nil || src == nil)
		return;

	/* Merge truth values using PLN revision formula */
	tvdst = atomgettruth(dst);
	tvsrc = atomgettruth(src);

	float wa = tvdst.confidence;
	float wb = tvsrc.confidence;
	float wtotal = wa + wb;

	if(wtotal > 0){
		tvmerged.strength = (tvdst.strength * wa + tvsrc.strength * wb) / wtotal;
		tvmerged.confidence = wtotal / (wtotal + 1);
	} else {
		tvmerged.strength = (tvdst.strength + tvsrc.strength) / 2.0;
		tvmerged.confidence = 0.0;
	}
	tvmerged.count = tvdst.count + tvsrc.count;

	atomsettruth(dst, tvmerged);
}

/*
 * Synchronize with remote via 9P file operations
 * Reads remote atomspace file, writes local changes to remote
 */
static int
machspace9psyncremote(MachSpace *ms, int idx)
{
	char path[512];
	int fd;
	AtomSpace *remote;
	int synced = 0;

	if(ms->hosts[idx] == nil)
		return 0;

	/* Import remote atomspace */
	snprint(path, sizeof(path), "%s/atomspace", ms->hosts[idx]);
	remote = atomspaceimportfile(path);

	if(remote != nil){
		/* Merge into local cache */
		if(ms->remote[idx] == nil)
			ms->remote[idx] = atomspacecreate();

		/* Merge remote into local */
		int i;
		for(i = 0; i < remote->natoms; i++){
			Atom *a = remote->atoms[i];
			if(a == nil)
				continue;

			Atom *existing = nil;
			if(a->name != nil){
				int j;
				for(j = 0; j < ms->local->natoms; j++){
					if(ms->local->atoms[j] && ms->local->atoms[j]->name &&
					   strcmp(ms->local->atoms[j]->name, a->name) == 0){
						existing = ms->local->atoms[j];
						break;
					}
				}
			}

			if(existing != nil){
				mergeatom(existing, a);
			} else {
				copyatom(ms->local, a);
			}
			synced++;
		}
		atomspacefree(remote);
	}

	/* Export local to remote */
	snprint(path, sizeof(path), "%s/atomspace", ms->hosts[idx]);
	atomspaceexportfile(ms->local, path);

	return synced;
}

/* Synchronize local and remote atomspaces via 9P */
int
machspacesync(MachSpace *ms)
{
	int i, j;
	Atom *local, *remote, *existing;
	int synced = 0;

	if(ms == nil)
		return -1;

	lock(ms);

	/* Sync each remote via 9P file operations */
	for(i = 0; i < ms->nremote; i++){
		/* Try 9P sync first */
		int n = machspace9psyncremote(ms, i);
		if(n > 0){
			synced += n;
			continue;
		}

		/* Fall back to in-memory sync if available */
		if(ms->remote[i] == nil)
			continue;

		/* Sync remote atoms to local */
		for(j = 0; j < ms->remote[i]->natoms; j++){
			remote = ms->remote[i]->atoms[j];
			if(remote == nil)
				continue;

			/* Check if atom exists locally */
			existing = atomfind(ms->local, remote->id);
			if(existing != nil){
				/* Merge truth values */
				mergeatom(existing, remote);
			} else {
				/* Copy atom to local */
				copyatom(ms->local, remote);
			}
			synced++;
		}

		/* Sync local atoms to remote */
		for(j = 0; j < ms->local->natoms; j++){
			local = ms->local->atoms[j];
			if(local == nil)
				continue;

			/* Check if atom exists in remote */
			existing = atomfind(ms->remote[i], local->id);
			if(existing != nil){
				/* Merge truth values */
				mergeatom(existing, local);
			} else {
				/* Copy atom to remote */
				copyatom(ms->remote[i], local);
			}
			synced++;
		}
	}

	unlock(ms);

	return synced;
}

/* Create an atom in MachSpace (local first, then sync) */
Atom*
machspacecreate(MachSpace *ms, int type, char *name)
{
	Atom *a;

	if(ms == nil || ms->local == nil)
		return nil;

	a = atomcreate(ms->local, type, name);
	return a;
}

/* Create a link in MachSpace */
Atom*
machspacelink(MachSpace *ms, int type, Atom **outgoing, int n)
{
	Atom *a;

	if(ms == nil || ms->local == nil)
		return nil;

	a = linkcreate(ms->local, type, outgoing, n);
	return a;
}

/* Query atoms across all spaces via 9P */
Atom**
machspacequery(MachSpace *ms, AtomPredicate pred, void *arg, int *n)
{
	Atom **results, **local, **remote;
	int nlocal, nremote, i, j, count, maxresults;

	if(ms == nil){
		*n = 0;
		return nil;
	}

	maxresults = 256;
	results = mallocz(sizeof(Atom*) * maxresults, 1);
	if(results == nil){
		*n = 0;
		return nil;
	}

	count = 0;

	/* Query local atomspace */
	local = atomquery(ms->local, pred, arg, &nlocal);
	if(local != nil){
		for(i = 0; i < nlocal && count < maxresults; i++){
			results[count++] = local[i];
		}
		free(local);
	}

	/* Query remote atomspaces */
	lock(ms);
	for(i = 0; i < ms->nremote; i++){
		if(ms->remote[i] == nil)
			continue;

		remote = atomquery(ms->remote[i], pred, arg, &nremote);
		if(remote != nil){
			for(j = 0; j < nremote && count < maxresults; j++){
				/* Avoid duplicates */
				int dup = 0;
				int k;
				for(k = 0; k < count; k++){
					if(results[k]->id == remote[j]->id){
						dup = 1;
						break;
					}
				}
				if(!dup){
					results[count++] = remote[j];
				}
			}
			free(remote);
		}
	}
	unlock(ms);

	*n = count;
	return results;
}

/*
 * Disconnect from remote and unmount
 */
int
machspace9pdisconnect(MachSpace *ms, char *host)
{
	int i;
	char mountpt[256];

	if(ms == nil || host == nil)
		return -1;

	snprint(mountpt, sizeof(mountpt), "%s/%s", basemountpt, host);

	lock(ms);
	for(i = 0; i < ms->nremote; i++){
		if(ms->hosts[i] && strcmp(ms->hosts[i], mountpt) == 0){
			unmount(nil, mountpt);
			free(ms->hosts[i]);
			ms->hosts[i] = nil;
			if(ms->remote[i]){
				atomspacefree(ms->remote[i]);
				ms->remote[i] = nil;
			}
			unlock(ms);
			return 0;
		}
	}
	unlock(ms);

	return -1;
}

/*
 * Get list of connected remote nodes
 */
char**
machspacenodes(MachSpace *ms, int *n)
{
	char **nodes;
	int count, i;

	if(ms == nil){
		*n = 0;
		return nil;
	}

	lock(ms);
	count = 0;
	for(i = 0; i < ms->nremote; i++){
		if(ms->hosts[i] != nil)
			count++;
	}

	nodes = mallocz(sizeof(char*) * count, 1);
	if(nodes == nil){
		unlock(ms);
		*n = 0;
		return nil;
	}

	count = 0;
	for(i = 0; i < ms->nremote; i++){
		if(ms->hosts[i] != nil)
			nodes[count++] = strdup(ms->hosts[i]);
	}
	unlock(ms);

	*n = count;
	return nodes;
}
