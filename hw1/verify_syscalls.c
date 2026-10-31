#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define __NR_revstr 451
#define __NR_tempbuf 452

enum mode { PRINT, ADD, REMOVE };

static void expect_errno(long result, int expected)
{
	assert(result == -1);
	assert(errno == expected);
}

int main(void)
{
	char buf[16];
	long ret;

	ret = syscall(__NR_revstr, NULL, 1);
	expect_errno(ret, EFAULT);

	ret = syscall(__NR_tempbuf, ADD, "dup", 0);
	expect_errno(ret, EFAULT);
	ret = syscall(__NR_tempbuf, ADD, NULL, 3);
	expect_errno(ret, EFAULT);

	ret = syscall(__NR_tempbuf, REMOVE, "missing", 7);
	expect_errno(ret, ENOENT);
	/* Clear the record left by the provided sample test, if it ran first. */
	(void)syscall(__NR_tempbuf, REMOVE, "Operating Systems", 17);
	assert(syscall(__NR_tempbuf, ADD, "dup", 3) == 0);
	assert(syscall(__NR_tempbuf, ADD, "dup", 3) == 0);

	memset(buf, 0, sizeof(buf));
	ret = syscall(__NR_tempbuf, PRINT, buf, 6);
	assert(ret == 5);
	assert(strcmp(buf, "dup d") == 0);

	assert(syscall(__NR_tempbuf, REMOVE, "dup", 3) == 0);
	memset(buf, 0xff, sizeof(buf));
	ret = syscall(__NR_tempbuf, PRINT, buf, sizeof(buf));
	assert(ret == 3);
	assert(strcmp(buf, "dup") == 0);
	assert(syscall(__NR_tempbuf, REMOVE, "dup", 3) == 0);

	memset(buf, 0xff, sizeof(buf));
	ret = syscall(__NR_tempbuf, PRINT, buf, sizeof(buf));
	assert(ret == 0);
	assert(buf[0] == '\0');

	puts("Extended syscall checks passed.");
	return 0;
}
