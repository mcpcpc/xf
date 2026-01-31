#ifndef TRANSFORM_H
#define TRANSFORM_H

#include <stddef.h>

struct buffer;
struct selection;

/* Replace occurrences of 'old' with 'new' in selected lines.
 * Modifies staged buffer. Returns number of replacements made. */
int transform_replace(struct buffer *staged, const struct selection *sel,
		const char *old, const char *new);

/* Delete selected lines. Returns number of lines deleted. */
int transform_delete(struct buffer *staged, const struct selection *sel);

/* Insert text before or after selected lines.
 * position: 0 = before first line of each range, 1 = after last line of each range
 * Returns number of lines inserted. */
int transform_insert(struct buffer *staged, const struct selection *sel,
		const char *text, int position);

/* Move selected lines to a target position.
 * target_line: 0-based line number where lines will be inserted
 * position: 0 = before target_line, 1 = after target_line
 * Returns number of lines moved, or -1 on error. */
int transform_move(struct buffer *staged, const struct selection *sel,
		size_t target_line, int position);

#endif /* TRANSFORM_H */
