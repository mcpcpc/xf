#ifndef HISTORY_H
#define HISTORY_H

#include <stddef.h>

#define HISTORY_MAX_ENTRIES 1024

/* Command history for replay and logging */
struct history {
	char **entries;
	size_t count;
	size_t capacity;
};

/* Initialize history */
void history_init(struct history *h);

/* Free history resources */
void history_free(struct history *h);

/* Log a command string */
int history_log(struct history *h, const char *cmd);

/* Save history to file */
int history_save(const struct history *h, const char *path);

/* Load history from file */
int history_load(struct history *h, const char *path);

/* Clear history */
void history_clear(struct history *h);

/* Get entry count */
size_t history_count(const struct history *h);

/* Get entry by index */
const char *history_get(const struct history *h, size_t index);

#endif /* HISTORY_H */
