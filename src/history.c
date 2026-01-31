#include "history.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void
history_init(struct history *h)
{
	h->entries = NULL;
	h->count = 0;
	h->capacity = 0;
}

void
history_free(struct history *h)
{
	size_t i;

	for (i = 0; i < h->count; i++)
		free(h->entries[i]);
	free(h->entries);
	h->entries = NULL;
	h->count = 0;
	h->capacity = 0;
}

void
history_clear(struct history *h)
{
	size_t i;

	for (i = 0; i < h->count; i++)
		free(h->entries[i]);
	h->count = 0;
}

int
history_log(struct history *h, const char *cmd)
{
	char *dup;
	size_t newcap;
	char **newentries;

	if (cmd == NULL || cmd[0] == '\0')
		return 0;

	if (h->count >= HISTORY_MAX_ENTRIES)
		return -1;

	if (h->count >= h->capacity) {
		newcap = h->capacity == 0 ? 64 : h->capacity * 2;
		if (newcap > HISTORY_MAX_ENTRIES)
			newcap = HISTORY_MAX_ENTRIES;
		newentries = realloc(h->entries, newcap * sizeof(char *));
		if (newentries == NULL)
			return -1;
		h->entries = newentries;
		h->capacity = newcap;
	}

	dup = strdup(cmd);
	if (dup == NULL)
		return -1;

	h->entries[h->count++] = dup;
	return 0;
}

int
history_save(const struct history *h, const char *path)
{
	FILE *fp;
	size_t i;

	fp = fopen(path, "w");
	if (fp == NULL)
		return -1;

	fprintf(fp, "# xf command history\n");
	for (i = 0; i < h->count; i++)
		fprintf(fp, "%s\n", h->entries[i]);

	fclose(fp);
	return 0;
}

int
history_load(struct history *h, const char *path)
{
	FILE *fp;
	char *line = NULL;
	size_t linecap = 0;
	ssize_t len;

	fp = fopen(path, "r");
	if (fp == NULL)
		return -1;

	while ((len = getline(&line, &linecap, fp)) != -1) {
		/* Strip newline */
		if (len > 0 && line[len - 1] == '\n')
			line[len - 1] = '\0';

		/* Skip empty lines and comments */
		if (line[0] == '\0' || line[0] == '#')
			continue;

		if (history_log(h, line) < 0)
			break;
	}

	free(line);
	fclose(fp);
	return 0;
}

size_t
history_count(const struct history *h)
{
	return h->count;
}

const char *
history_get(const struct history *h, size_t index)
{
	if (index >= h->count)
		return NULL;
	return h->entries[index];
}
