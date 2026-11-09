// SPDX-License-Identifier: GPL-2.0
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

SYSCALL_DEFINE2(revstr, char __user *, str, size_t, n)
{
	char *kstr;
	size_t i;
	long ret = 0;

	if (n == (size_t)-1)
		return -ENOMEM;
	kstr = kmalloc(n + 1, GFP_KERNEL);
	if (!kstr)
		return -ENOMEM;

	if (copy_from_user(kstr, str, n)) {
		ret = -EFAULT;
		goto out;
	}
	kstr[n] = '\0';
	pr_info("The origin string: %s\n", kstr);

	for (i = 0; i < n / 2; i++) {
		char tmp = kstr[i];

		kstr[i] = kstr[n - i - 1];
		kstr[n - i - 1] = tmp;
	}
	pr_info("The reversed string: %s\n", kstr);
	if (copy_to_user(str, kstr, n))
		ret = -EFAULT;
out:
	kfree(kstr);
	return ret;
}
