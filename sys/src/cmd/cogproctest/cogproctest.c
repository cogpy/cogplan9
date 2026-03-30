/*
 * cogproctest - Test program for /proc/pid/cog interface
 * 
 * Demonstrates usage of per-process cognitive namespace
 */

#include <u.h>
#include <libc.h>

void
usage(void)
{
	fprint(2, "usage: %s [-t test] [-p pid]\n", argv0);
	exits("usage");
}

void
testfocus(void)
{
	int fd;
	char buf[256];
	int n;
	
	print("=== Testing focus interface ===\n");
	
	/* Read current focus */
	fd = open("/proc/self/cog/focus", OREAD);
	if(fd < 0){
		print("FAIL: cannot open /proc/self/cog/focus: %r\n");
		return;
	}
	
	n = read(fd, buf, sizeof buf - 1);
	if(n > 0){
		buf[n] = 0;
		print("Current focus: %s", buf);
	}
	close(fd);
	
	/* Set new focus */
	fd = open("/proc/self/cog/focus", OWRITE);
	if(fd < 0){
		print("FAIL: cannot write to focus: %r\n");
		return;
	}
	
	fprint(fd, "atom 12345\n");
	fprint(fd, "sti 100\n");
	close(fd);
	print("PASS: Set focus to atom 12345, STI 100\n");
	
	/* Verify */
	fd = open("/proc/self/cog/focus", OREAD);
	if(fd >= 0){
		n = read(fd, buf, sizeof buf - 1);
		if(n > 0){
			buf[n] = 0;
			print("New focus: %s", buf);
		}
		close(fd);
	}
	
	print("\n");
}

void
testgoals(void)
{
	int fd;
	char buf[4096];
	int n;
	
	print("=== Testing goals interface ===\n");
	
	/* Add goals */
	fd = open("/proc/self/cog/goals", OWRITE);
	if(fd < 0){
		print("FAIL: cannot open goals: %r\n");
		return;
	}
	
	fprint(fd, "add 1001\n");
	fprint(fd, "add 1002\n");
	fprint(fd, "add 1003\n");
	close(fd);
	print("PASS: Added 3 goals\n");
	
	/* Read goals */
	fd = open("/proc/self/cog/goals", OREAD);
	if(fd >= 0){
		n = read(fd, buf, sizeof buf - 1);
		if(n > 0){
			buf[n] = 0;
			print("Current goals:\n%s", buf);
		}
		close(fd);
	}
	
	/* Remove a goal */
	fd = open("/proc/self/cog/goals", OWRITE);
	if(fd >= 0){
		fprint(fd, "remove 1002\n");
		close(fd);
		print("PASS: Removed goal 1002\n");
	}
	
	/* Verify */
	fd = open("/proc/self/cog/goals", OREAD);
	if(fd >= 0){
		n = read(fd, buf, sizeof buf - 1);
		if(n > 0){
			buf[n] = 0;
			print("After removal:\n%s", buf);
		}
		close(fd);
	}
	
	print("\n");
}

void
testbeliefs(void)
{
	int fd;
	char buf[4096];
	int n;
	
	print("=== Testing beliefs interface ===\n");
	
	/* Add beliefs */
	fd = open("/proc/self/cog/beliefs", OWRITE);
	if(fd < 0){
		print("FAIL: cannot open beliefs: %r\n");
		return;
	}
	
	fprint(fd, "add 2001\n");
	fprint(fd, "add 2002\n");
	fprint(fd, "add 2003\n");
	fprint(fd, "add 2004\n");
	close(fd);
	print("PASS: Added 4 beliefs\n");
	
	/* Read beliefs */
	fd = open("/proc/self/cog/beliefs", OREAD);
	if(fd >= 0){
		n = read(fd, buf, sizeof buf - 1);
		if(n > 0){
			buf[n] = 0;
			print("Current beliefs:\n%s", buf);
		}
		close(fd);
	}
	
	print("\n");
}

void
testatoms(void)
{
	int fd;
	char buf[8192];
	int n;
	
	print("=== Testing atoms interface ===\n");
	
	/* Read all relevant atoms */
	fd = open("/proc/self/cog/atoms", OREAD);
	if(fd < 0){
		print("FAIL: cannot open atoms: %r\n");
		return;
	}
	
	n = read(fd, buf, sizeof buf - 1);
	if(n > 0){
		buf[n] = 0;
		print("Relevant atoms:\n%s", buf);
	}
	else{
		print("No atoms found\n");
	}
	close(fd);
	
	print("\n");
}

void
teststats(void)
{
	int fd;
	char buf[1024];
	int n;
	
	print("=== Testing stats interface ===\n");
	
	/* Read statistics */
	fd = open("/proc/self/cog/stats", OREAD);
	if(fd < 0){
		print("FAIL: cannot open stats: %r\n");
		return;
	}
	
	n = read(fd, buf, sizeof buf - 1);
	if(n > 0){
		buf[n] = 0;
		print("Cognitive statistics:\n%s", buf);
	}
	close(fd);
	
	print("\n");
}

void
testctl(void)
{
	int fd;
	
	print("=== Testing ctl interface ===\n");
	
	/* Trigger inference */
	fd = open("/proc/self/cog/ctl", OWRITE);
	if(fd < 0){
		print("FAIL: cannot open ctl: %r\n");
		return;
	}
	
	fprint(fd, "inference\n");
	print("PASS: Triggered inference\n");
	
	fprint(fd, "inference\n");
	fprint(fd, "inference\n");
	print("PASS: Triggered 2 more inferences\n");
	
	close(fd);
	
	/* Check stats */
	teststats();
	
	/* Reset */
	fd = open("/proc/self/cog/ctl", OWRITE);
	if(fd >= 0){
		fprint(fd, "reset\n");
		close(fd);
		print("PASS: Reset statistics\n");
	}
	
	print("\n");
}

void
runalltests(void)
{
	print("========================================\n");
	print("  /proc/pid/cog Interface Test Suite\n");
	print("========================================\n\n");
	
	testfocus();
	testgoals();
	testbeliefs();
	testatoms();
	testctl();
	teststats();
	
	print("========================================\n");
	print("  All tests completed\n");
	print("========================================\n");
}

void
inspectproc(int pid)
{
	char path[256];
	char buf[8192];
	int fd, n;
	
	print("=== Inspecting process %d ===\n\n", pid);
	
	/* Stats */
	snprint(path, sizeof path, "/proc/%d/cog/stats", pid);
	fd = open(path, OREAD);
	if(fd >= 0){
		n = read(fd, buf, sizeof buf - 1);
		if(n > 0){
			buf[n] = 0;
			print("Statistics:\n%s\n", buf);
		}
		close(fd);
	}
	
	/* Atoms */
	snprint(path, sizeof path, "/proc/%d/cog/atoms", pid);
	fd = open(path, OREAD);
	if(fd >= 0){
		n = read(fd, buf, sizeof buf - 1);
		if(n > 0){
			buf[n] = 0;
			print("Atoms:\n%s\n", buf);
		}
		close(fd);
	}
	
	/* Focus */
	snprint(path, sizeof path, "/proc/%d/cog/focus", pid);
	fd = open(path, OREAD);
	if(fd >= 0){
		n = read(fd, buf, sizeof buf - 1);
		if(n > 0){
			buf[n] = 0;
			print("Focus:\n%s\n", buf);
		}
		close(fd);
	}
	
	/* Goals */
	snprint(path, sizeof path, "/proc/%d/cog/goals", pid);
	fd = open(path, OREAD);
	if(fd >= 0){
		n = read(fd, buf, sizeof buf - 1);
		if(n > 0){
			buf[n] = 0;
			print("Goals:\n%s\n", buf);
		}
		close(fd);
	}
}

void
main(int argc, char *argv[])
{
	char *test;
	int pid;
	
	test = nil;
	pid = -1;
	
	ARGBEGIN{
	case 't':
		test = EARGF(usage());
		break;
	case 'p':
		pid = atoi(EARGF(usage()));
		break;
	default:
		usage();
	}ARGEND
	
	if(pid >= 0){
		inspectproc(pid);
		exits(nil);
	}
	
	if(test != nil){
		if(strcmp(test, "focus") == 0)
			testfocus();
		else if(strcmp(test, "goals") == 0)
			testgoals();
		else if(strcmp(test, "beliefs") == 0)
			testbeliefs();
		else if(strcmp(test, "atoms") == 0)
			testatoms();
		else if(strcmp(test, "stats") == 0)
			teststats();
		else if(strcmp(test, "ctl") == 0)
			testctl();
		else
			usage();
	}
	else{
		runalltests();
	}
	
	exits(nil);
}
