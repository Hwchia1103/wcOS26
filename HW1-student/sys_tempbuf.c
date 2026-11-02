// SPDX-License-Identifier: GPL-2.0
#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/overflow.h>
#include <linux/err.h>
#include <linux/string.h>

enum hw1_tempbuf_mode {
	HW1_TEMPBUF_PRINT = 0,
	HW1_TEMPBUF_ADD = 1,
	HW1_TEMPBUF_REMOVE = 2,
};

struct hw1_tempbuf_record {
	struct list_head list;
	size_t len;
	char text[];
};

static LIST_HEAD(hw1_tempbuf_records);
static DEFINE_MUTEX(hw1_tempbuf_lock);

static char *hw1_copy_input(const char __user *data, size_t size)
{
	char *text;

	if (!data || !size)
		return ERR_PTR(-EFAULT);
	if (size == (size_t)-1)
		return ERR_PTR(-ENOMEM);

	text = kmalloc(size + 1, GFP_KERNEL);
	if (!text)
		return ERR_PTR(-ENOMEM);
	if (copy_from_user(text, data, size)) {
		kfree(text);
		return ERR_PTR(-EFAULT);
	}
	text[size] = '\0';
	return text;
}

SYSCALL_DEFINE3(tempbuf, int, mode, char __user *, data, size_t, size)
{
	struct hw1_tempbuf_record *record, *next;
	char *input;
	long ret = 0;

	if (!data || !size)
		return -EFAULT;

	if (mode == HW1_TEMPBUF_ADD || mode == HW1_TEMPBUF_REMOVE) {
		input = hw1_copy_input(data, size);
		if (IS_ERR(input))
			return PTR_ERR(input);

		mutex_lock(&hw1_tempbuf_lock);
		if (mode == HW1_TEMPBUF_ADD) {
			record = kmalloc(struct_size(record, text, size + 1), GFP_KERNEL);
			if (!record) {
				ret = -ENOMEM;
				goto unlock_input;
			}
			INIT_LIST_HEAD(&record->list);
			record->len = size;
			memcpy(record->text, input, size + 1);
			list_add_tail(&record->list, &hw1_tempbuf_records);
			pr_info("[tempbuf] Added: %s\n", input);
		} else {
			list_for_each_entry_safe(record, next, &hw1_tempbuf_records, list) {
				if (record->len == size && !memcmp(record->text, input, size)) {
					list_del(&record->list);
					kfree(record);
					pr_info("[tempbuf] Removed: %s\n", input);
					goto unlock_input;
				}
			}
			ret = -ENOENT;
		}
unlock_input:
		mutex_unlock(&hw1_tempbuf_lock);
		kfree(input);
		return ret;
	}

	if (mode != HW1_TEMPBUF_PRINT)
		return -EINVAL;

	mutex_lock(&hw1_tempbuf_lock);
	{
		size_t total = 0, copied, offset = 0;
		bool first = true;
		char *joined;

		list_for_each_entry(record, &hw1_tempbuf_records, list) {
			if (check_add_overflow(total, record->len, &total) ||
			    (!first && check_add_overflow(total, (size_t)1, &total))) {
				ret = -ENOMEM;
				goto print_unlock;
			}
			first = false;
		}
		if (total == (size_t)-1) {
			ret = -ENOMEM;
			goto print_unlock;
		}
		joined = kmalloc(total + 1, GFP_KERNEL);
		if (!joined) {
			ret = -ENOMEM;
			goto print_unlock;
		}
		first = true;
		list_for_each_entry(record, &hw1_tempbuf_records, list) {
			if (!first)
				joined[offset++] = ' ';
			memcpy(joined + offset, record->text, record->len);
			offset += record->len;
			first = false;
		}
		joined[total] = '\0';
		pr_info("[tempbuf] %s\n", joined);
		copied = min(total, size - 1);
		if (copy_to_user(data, joined, copied) || put_user('\0', data + copied))
			ret = -EFAULT;
		else
			ret = copied;
		kfree(joined);
	}
print_unlock:
	mutex_unlock(&hw1_tempbuf_lock);
	return ret;
}
