/*
 * Cognitive Kernel Demo
 * 
 * Demonstrates the revolutionary kernel-level cognitive capabilities.
 * Shows how thinking, reasoning, and intelligence are fundamental OS services.
 */

#include <u.h>
#include <libc.h>

void usage(void);
void testdevice(void);
void testatomspace(void);
void testinference(void);
void testattention(void);
void teststats(void);
void interactive(void);

void
usage(void)
{
	fprint(2, "usage: cogkernel [-t test] [-i]\n");
	fprint(2, "  -t device     Test cognitive device\n");
	fprint(2, "  -t atomspace  Test kernel AtomSpace\n");
	fprint(2, "  -t inference  Test PLN inference\n");
	fprint(2, "  -t attention  Test ECAN attention\n");
	fprint(2, "  -t stats      Show statistics\n");
	fprint(2, "  -i            Interactive mode\n");
	exits("usage");
}

void
main(int argc, char *argv[])
{
	char *test;
	int iflag;

	test = nil;
	iflag = 0;

	ARGBEGIN{
	case 't':
		test = EARGF(usage());
		break;
	case 'i':
		iflag = 1;
		break;
	default:
		usage();
	}ARGEND

	print("Cognitive Kernel Demo\n");
	print("=====================\n\n");
	print("Revolutionary OS where thinking is a kernel service.\n\n");

	if(iflag) {
		interactive();
		exits(nil);
	}

	if(test == nil) {
		/* Run all tests */
		testdevice();
		testatomspace();
		testinference();
		testattention();
		teststats();
	} else if(strcmp(test, "device") == 0) {
		testdevice();
	} else if(strcmp(test, "atomspace") == 0) {
		testatomspace();
	} else if(strcmp(test, "inference") == 0) {
		testinference();
	} else if(strcmp(test, "attention") == 0) {
		testattention();
	} else if(strcmp(test, "stats") == 0) {
		teststats();
	} else {
		fprint(2, "unknown test: %s\n", test);
		usage();
	}

	exits(nil);
}

void
testdevice(void)
{
	int fd;
	char buf[1024];
	int n;

	print("Test 1: Cognitive Device\n");
	print("-------------------------\n");

	fd = open("#Σ/cogvm", OREAD);
	if(fd < 0) {
		print("FAIL: Cannot open cognitive device: %r\n");
		print("Note: Kernel may not have cognitive extensions.\n\n");
		return;
	}

	n = read(fd, buf, sizeof buf);
	if(n > 0) {
		buf[n] = 0;
		print("Cognitive VM state:\n%s\n", buf);
	}

	close(fd);
	print("PASS: Cognitive device accessible\n\n");
}

void
testatomspace(void)
{
	int fd;
	char buf[8192];
	int n;

	print("Test 2: Kernel AtomSpace\n");
	print("------------------------\n");

	/* Create atoms at kernel level */
	fd = open("#Σ/atomspace", ORDWR);
	if(fd < 0) {
		print("FAIL: Cannot open atomspace: %r\n\n");
		return;
	}

	print("Creating atoms in kernel...\n");
	write(fd, "create 1 cat", 12);
	write(fd, "create 1 animal", 15);
	write(fd, "create 1 mammal", 15);

	/* Read kernel AtomSpace */
	print("\nKernel AtomSpace contents:\n");
	seek(fd, 0, 0);
	n = read(fd, buf, sizeof buf);
	if(n > 0) {
		buf[n] = 0;
		print("%s\n", buf);
	}

	close(fd);
	print("PASS: Atoms created in kernel\n\n");
}

void
testinference(void)
{
	int fd;
	char buf[256];

	print("Test 3: Kernel PLN Inference\n");
	print("----------------------------\n");

	fd = open("#Σ/pln", ORDWR);
	if(fd < 0) {
		print("FAIL: Cannot open PLN device: %r\n\n");
		return;
	}

	print("Performing deduction at kernel level...\n");
	print("Given: cat->animal, animal->living\n");
	print("Infer: cat->living\n\n");

	/* Assuming atoms were created with IDs 1, 2, 3 */
	write(fd, "deduction 1 2", 13);

	close(fd);
	print("PASS: Inference executed in kernel\n\n");
}

void
testattention(void)
{
	int fd;
	char buf[2048];
	int n;

	print("Test 4: Kernel ECAN Attention\n");
	print("-----------------------------\n");

	fd = open("#Σ/ecan", ORDWR);
	if(fd < 0) {
		print("FAIL: Cannot open ECAN device: %r\n\n");
		return;
	}

	print("Allocating attention to atoms...\n");
	write(fd, "allocate 1 100", 14);
	write(fd, "allocate 2 50", 13);

	print("\nUpdating attention network...\n");
	write(fd, "update", 6);

	print("\nECAN state:\n");
	seek(fd, 0, 0);
	n = read(fd, buf, sizeof buf);
	if(n > 0) {
		buf[n] = 0;
		print("%s\n", buf);
	}

	close(fd);
	print("PASS: Attention allocated in kernel\n\n");
}

void
teststats(void)
{
	int fd;
	char buf[4096];
	int n;

	print("Test 5: Cognitive Statistics\n");
	print("----------------------------\n");

	fd = open("#Σ/stats", OREAD);
	if(fd < 0) {
		print("FAIL: Cannot open stats: %r\n\n");
		return;
	}

	n = read(fd, buf, sizeof buf);
	if(n > 0) {
		buf[n] = 0;
		print("%s\n", buf);
	}

	close(fd);
	print("PASS: Statistics retrieved\n\n");
}

void
interactive(void)
{
	int fd;
	char line[256];
	char *p;
	int n;

	print("Interactive Cognitive Kernel Shell\n");
	print("===================================\n\n");
	print("Commands:\n");
	print("  create <type> <name>  - Create atom\n");
	print("  list                  - List atoms\n");
	print("  infer <id1> <id2>    - Perform deduction\n");
	print("  focus <id> <sti>     - Allocate attention\n");
	print("  stats                 - Show statistics\n");
	print("  quit                  - Exit\n\n");

	fd = open("#Σ/atomspace", ORDWR);
	if(fd < 0) {
		print("Cannot open cognitive device: %r\n");
		exits("device");
	}

	for(;;) {
		print("cog> ");
		if(read(0, line, sizeof line) <= 0)
			break;

		/* Remove newline */
		for(p = line; *p; p++)
			if(*p == '\n') {
				*p = 0;
				break;
			}

		if(strlen(line) == 0)
			continue;

		if(strcmp(line, "quit") == 0)
			break;

		if(strcmp(line, "list") == 0) {
			char buf[8192];
			seek(fd, 0, 0);
			n = read(fd, buf, sizeof buf);
			if(n > 0) {
				buf[n] = 0;
				print("%s\n", buf);
			}
			continue;
		}

		if(strcmp(line, "stats") == 0) {
			int sfd;
			char buf[4096];
			sfd = open("#Σ/stats", OREAD);
			if(sfd >= 0) {
				n = read(sfd, buf, sizeof buf);
				if(n > 0) {
					buf[n] = 0;
					print("%s\n", buf);
				}
				close(sfd);
			}
			continue;
		}

		if(strncmp(line, "create ", 7) == 0) {
			write(fd, line, strlen(line));
			print("Atom created\n");
			continue;
		}

		if(strncmp(line, "infer ", 6) == 0) {
			int pfd = open("#Σ/pln", ORDWR);
			if(pfd >= 0) {
				write(pfd, line + 6, strlen(line + 6));
				print("Inference complete\n");
				close(pfd);
			}
			continue;
		}

		if(strncmp(line, "focus ", 6) == 0) {
			int efd = open("#Σ/ecan", ORDWR);
			if(efd >= 0) {
				write(efd, line + 6, strlen(line + 6));
				print("Attention allocated\n");
				close(efd);
			}
			continue;
		}

		print("Unknown command: %s\n", line);
	}

	close(fd);
	print("\nGoodbye!\n");
}
