#include "kvm/guest_compat.h"

#include "kvm/mutex.h"

#include <linux/kernel.h>
#include <linux/list.h>

struct compat_message {
	int id;
	char *title;
	char *desc;

	struct list_head list;
};

static int id;
static DEFINE_MUTEX(compat_mtx);
static LIST_HEAD(messages);

int compat__add_message(const char *title, const char *desc)
{
	struct compat_message *msg;

	mutex_lock(&compat_mtx);
	msg = malloc(sizeof(*msg));
	if (msg == NULL)
		goto cleanup;

	*msg = (struct compat_message) {
		.id = id,
		.title = strdup(title),
		.desc = strdup(desc),
	};

	if (msg->title == NULL || msg->desc == NULL)
		goto cleanup;

	list_add_tail(&msg->list, &messages);

	mutex_unlock(&compat_mtx);

	return id++;

cleanup:
	if (msg) {
		free(msg->title);
		free(msg->desc);
		free(msg);
	}

	mutex_unlock(&compat_mtx);

	return -ENOMEM;
}

static void compat__free(struct compat_message *msg)
{
	free(msg->title);
	free(msg->desc);
	free(msg);
}

int compat__remove_message(int id)
{
	struct compat_message *pos, *n;

	mutex_lock(&compat_mtx);

	list_for_each_entry_safe(pos, n, &messages, list) {
		if (pos->id == id) {
			list_del(&pos->list);
			compat__free(pos);

			mutex_unlock(&compat_mtx);

			return 0;
		}
	}

	mutex_unlock(&compat_mtx);

	return -ENOENT;
}

int compat__print_all_messages(void)
{
	mutex_lock(&compat_mtx);

	while (!list_empty(&messages)) {
		struct compat_message *msg;

		msg = list_first_entry(&messages, struct compat_message, list);

		printf("\n\n*** Compatibility Warning ***\n\n\t%s\n\n%s\n",
			msg->title, msg->desc);

		list_del(&msg->list);
		compat__free(msg);
	}

	mutex_unlock(&compat_mtx);

	return 0;
}