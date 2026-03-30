/*
 * AtomSpace serialization for Plan9Cog
 * Supports persistence via any Plan 9 file descriptor
 * (local files, network connections, pipes)
 */

#include <u.h>
#include <libc.h>
#include <bio.h>
#include <plan9cog/atomspace.h>

/*
 * File format:
 *   ATOMSPACE <natoms>
 *   NODE <id> <type> <name> <strength> <confidence> <count> <sti> <lti> <vlti>
 *   LINK <id> <type> <noutgoing> <outgoing_ids...> <strength> <confidence> <count> <sti> <lti> <vlti>
 *   END
 */

int
atomspaceexport(AtomSpace *as, int fd)
{
	int i, j;
	Atom *a;
	char buf[8192];
	int n;

	lock(as);

	/* Write header */
	n = snprint(buf, sizeof(buf), "ATOMSPACE %d\n", as->natoms);
	if(write(fd, buf, n) != n){
		unlock(as);
		return -1;
	}

	/* First pass: write all nodes (atoms without outgoing links) */
	for(i = 0; i < as->natoms; i++){
		a = as->atoms[i];
		if(a == nil)
			continue;

		if(a->noutgoing == 0 || a->outgoing == nil){
			n = snprint(buf, sizeof(buf), "NODE %ld %d %s %f %f %ld %d %d %d\n",
				a->id, a->type,
				a->name ? a->name : "_",
				a->tv.strength, a->tv.confidence, a->tv.count,
				a->av.sti, a->av.lti, a->av.vlti);

			if(write(fd, buf, n) != n){
				unlock(as);
				return -1;
			}
		}
	}

	/* Second pass: write all links (atoms with outgoing links) */
	for(i = 0; i < as->natoms; i++){
		a = as->atoms[i];
		if(a == nil)
			continue;

		if(a->noutgoing > 0 && a->outgoing != nil){
			n = snprint(buf, sizeof(buf), "LINK %ld %d %d",
				a->id, a->type, a->noutgoing);

			/* Write outgoing atom IDs */
			for(j = 0; j < a->noutgoing; j++){
				n += snprint(buf + n, sizeof(buf) - n, " %ld",
					a->outgoing[j] ? a->outgoing[j]->id : 0);
			}

			n += snprint(buf + n, sizeof(buf) - n, " %f %f %ld %d %d %d\n",
				a->tv.strength, a->tv.confidence, a->tv.count,
				a->av.sti, a->av.lti, a->av.vlti);

			if(write(fd, buf, n) != n){
				unlock(as);
				return -1;
			}
		}
	}

	/* Write end marker */
	n = snprint(buf, sizeof(buf), "END\n");
	if(write(fd, buf, n) != n){
		unlock(as);
		return -1;
	}

	unlock(as);
	return 0;
}

/* ID mapping for import - maps old IDs to new atoms */
typedef struct IdMap IdMap;
struct IdMap {
	ulong oldid;
	Atom *atom;
};

static Atom*
findbyoldid(IdMap *map, int nmap, ulong oldid)
{
	int i;
	for(i = 0; i < nmap; i++){
		if(map[i].oldid == oldid)
			return map[i].atom;
	}
	return nil;
}

AtomSpace*
atomspaceimport(int fd)
{
	AtomSpace *as;
	Biobuf *bp;
	char *line;
	char *fields[32];
	int nfields;
	int natoms;
	IdMap *idmap;
	int nidmap, maxidmap;
	Atom *a;
	Atom **outgoing;
	int i, nout;
	ulong oldid;
	TruthValue tv;
	AttentionValue av;

	bp = Bfdopen(fd, OREAD);
	if(bp == nil)
		return nil;

	/* Read header */
	line = Brdstr(bp, '\n', 1);
	if(line == nil || strncmp(line, "ATOMSPACE ", 10) != 0){
		free(line);
		Bterm(bp);
		return nil;
	}

	natoms = atoi(line + 10);
	free(line);

	as = atomspacecreate();
	if(as == nil){
		Bterm(bp);
		return nil;
	}

	/* Allocate ID mapping table */
	maxidmap = natoms > 0 ? natoms : 128;
	idmap = mallocz(sizeof(IdMap) * maxidmap, 1);
	if(idmap == nil){
		atomspacefree(as);
		Bterm(bp);
		return nil;
	}
	nidmap = 0;

	/* Read atoms */
	while((line = Brdstr(bp, '\n', 1)) != nil){
		if(strcmp(line, "END") == 0){
			free(line);
			break;
		}

		/* Parse line into fields */
		nfields = tokenize(line, fields, nelem(fields));
		if(nfields < 2){
			free(line);
			continue;
		}

		if(strcmp(fields[0], "NODE") == 0 && nfields >= 10){
			/* NODE <id> <type> <name> <strength> <confidence> <count> <sti> <lti> <vlti> */
			oldid = strtoul(fields[1], nil, 10);

			a = atomcreate(as, atoi(fields[2]),
				strcmp(fields[3], "_") == 0 ? nil : fields[3]);

			if(a != nil){
				tv.strength = atof(fields[4]);
				tv.confidence = atof(fields[5]);
				tv.count = strtoul(fields[6], nil, 10);
				atomsettruth(a, tv);

				av.sti = atoi(fields[7]);
				av.lti = atoi(fields[8]);
				av.vlti = atoi(fields[9]);
				atomsetattention(a, av);

				/* Add to ID map */
				if(nidmap >= maxidmap){
					maxidmap *= 2;
					idmap = realloc(idmap, sizeof(IdMap) * maxidmap);
				}
				idmap[nidmap].oldid = oldid;
				idmap[nidmap].atom = a;
				nidmap++;
			}
		}
		else if(strcmp(fields[0], "LINK") == 0 && nfields >= 4){
			/* LINK <id> <type> <noutgoing> <outgoing_ids...> <strength> <confidence> <count> <sti> <lti> <vlti> */
			oldid = strtoul(fields[1], nil, 10);
			nout = atoi(fields[3]);

			if(nfields >= 4 + nout + 6){
				/* Resolve outgoing atoms */
				outgoing = mallocz(sizeof(Atom*) * nout, 1);
				if(outgoing != nil){
					for(i = 0; i < nout; i++){
						ulong outid = strtoul(fields[4 + i], nil, 10);
						outgoing[i] = findbyoldid(idmap, nidmap, outid);
					}

					a = linkcreate(as, atoi(fields[2]), outgoing, nout);
					free(outgoing);

					if(a != nil){
						int tvbase = 4 + nout;
						tv.strength = atof(fields[tvbase]);
						tv.confidence = atof(fields[tvbase + 1]);
						tv.count = strtoul(fields[tvbase + 2], nil, 10);
						atomsettruth(a, tv);

						av.sti = atoi(fields[tvbase + 3]);
						av.lti = atoi(fields[tvbase + 4]);
						av.vlti = atoi(fields[tvbase + 5]);
						atomsetattention(a, av);

						/* Add to ID map */
						if(nidmap >= maxidmap){
							maxidmap *= 2;
							idmap = realloc(idmap, sizeof(IdMap) * maxidmap);
						}
						idmap[nidmap].oldid = oldid;
						idmap[nidmap].atom = a;
						nidmap++;
					}
				}
			}
		}

		free(line);
	}

	free(idmap);
	Bterm(bp);
	return as;
}

/*
 * Export to named file - Plan 9 style
 * Supports network paths like /n/remote/atomspace
 */
int
atomspaceexportfile(AtomSpace *as, char *path)
{
	int fd;
	int ret;

	fd = create(path, OWRITE, 0644);
	if(fd < 0)
		return -1;

	ret = atomspaceexport(as, fd);
	close(fd);
	return ret;
}

/*
 * Import from named file - Plan 9 style
 * Supports network paths like /n/remote/atomspace
 */
AtomSpace*
atomspaceimportfile(char *path)
{
	int fd;
	AtomSpace *as;

	fd = open(path, OREAD);
	if(fd < 0)
		return nil;

	as = atomspaceimport(fd);
	close(fd);
	return as;
}

/*
 * Merge imported atomspace into existing one
 * Used for distributed cognition - merge remote knowledge
 */
int
atomspacemerge(AtomSpace *dst, AtomSpace *src)
{
	int i;
	Atom *a, *existing;
	TruthValue tvdst, tvsrc, tvmerged;

	if(dst == nil || src == nil)
		return -1;

	lock(dst);
	lock(src);

	for(i = 0; i < src->natoms; i++){
		a = src->atoms[i];
		if(a == nil)
			continue;

		/* Check if atom exists in destination by name (for nodes) */
		existing = nil;
		if(a->name != nil){
			int j;
			for(j = 0; j < dst->natoms; j++){
				if(dst->atoms[j] && dst->atoms[j]->name &&
				   dst->atoms[j]->type == a->type &&
				   strcmp(dst->atoms[j]->name, a->name) == 0){
					existing = dst->atoms[j];
					break;
				}
			}
		}

		if(existing != nil){
			/* Merge truth values using revision formula */
			tvdst = atomgettruth(existing);
			tvsrc = atomgettruth(a);

			float wa = tvdst.confidence;
			float wb = tvsrc.confidence;
			float wtotal = wa + wb;

			if(wtotal > 0){
				tvmerged.strength = (tvdst.strength * wa + tvsrc.strength * wb) / wtotal;
				tvmerged.confidence = wtotal / (wtotal + 1);
			} else {
				tvmerged.strength = (tvdst.strength + tvsrc.strength) / 2;
				tvmerged.confidence = 0.0;
			}
			tvmerged.count = tvdst.count + tvsrc.count;

			atomsettruth(existing, tvmerged);
		} else {
			/* Copy atom to destination */
			if(a->noutgoing == 0){
				atomcreate(dst, a->type, a->name);
			}
			/* Links would need special handling to resolve references */
		}
	}

	unlock(src);
	unlock(dst);

	return 0;
}
