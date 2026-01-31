#include "transform.h"
#include "buffer.h"
#include "selection.h"

#include <stdlib.h>
#include <string.h>

static int
is_line_selected(const struct selection *sel, size_t line)
{
	size_t i;

	for (i = 0; i < sel->nranges; i++) {
		if (line >= sel->ranges[i].start && line <= sel->ranges[i].end)
			return 1;
	}
	return 0;
}

static char *
str_replace(const char *haystack, const char *needle, const char *replacement)
{
	const char *p;
	char *result, *dst;
	size_t needle_len, repl_len, count, result_len;

	needle_len = strlen(needle);
	repl_len = strlen(replacement);

	/* Count occurrences */
	count = 0;
	p = haystack;
	while ((p = strstr(p, needle)) != NULL) {
		count++;
		p += needle_len;
	}

	if (count == 0)
		return strdup(haystack);

	/* Calculate result length */
	result_len = strlen(haystack) + count * (repl_len - needle_len);
	result = malloc(result_len + 1);
	if (result == NULL)
		return NULL;

	/* Build result */
	dst = result;
	p = haystack;
	while (*p) {
		if (strncmp(p, needle, needle_len) == 0) {
			memcpy(dst, replacement, repl_len);
			dst += repl_len;
			p += needle_len;
		} else {
			*dst++ = *p++;
		}
	}
	*dst = '\0';

	return result;
}

int
transform_replace(struct buffer *staged, const struct selection *sel,
		  const char *old, const char *new)
{
	size_t i;
	int count = 0;

	for (i = 0; i < staged->nlines; i++) {
		if (is_line_selected(sel, i)) {
			char *replaced;

			if (strstr(staged->lines[i].text, old) == NULL)
				continue;

			replaced = str_replace(staged->lines[i].text, old, new);
			if (replaced == NULL)
				return -1;

			free(staged->lines[i].text);
			staged->lines[i].text = replaced;
			staged->lines[i].len = strlen(replaced);
			count++;
		}
	}

	return count;
}

int
transform_delete(struct buffer *staged, const struct selection *sel)
{
	struct line *newlines;
	size_t i, j, newcount;
	int deleted = 0;

	/* Count lines to keep */
	newcount = 0;
	for (i = 0; i < staged->nlines; i++) {
		if (!is_line_selected(sel, i))
			newcount++;
		else
			deleted++;
	}

	if (deleted == 0)
		return 0;

	newlines = malloc(newcount * sizeof(struct line));
	if (newlines == NULL)
		return -1;

	j = 0;
	for (i = 0; i < staged->nlines; i++) {
		if (!is_line_selected(sel, i)) {
			newlines[j].text = staged->lines[i].text;
			newlines[j].len = staged->lines[i].len;
			j++;
		} else {
			free(staged->lines[i].text);
		}
	}

	free(staged->lines);
	staged->lines = newlines;
	staged->nlines = newcount;
	staged->cap = newcount;

	return deleted;
}

int
transform_insert(struct buffer *staged, const struct selection *sel,
		 const char *text, int position)
{
	struct line *newlines;
	size_t i, j, r;
	size_t inserted = 0;
	size_t newcount;
	size_t *insert_points;
	size_t ninserts;

	if (sel->nranges == 0 || text == NULL)
		return 0;

	/* Collect insertion points */
	insert_points = malloc(sel->nranges * sizeof(size_t));
	if (insert_points == NULL)
		return -1;

	ninserts = 0;
	for (r = 0; r < sel->nranges; r++) {
		size_t pt;
		if (position == 0)
			pt = sel->ranges[r].start;
		else
			pt = sel->ranges[r].end + 1;
		insert_points[ninserts++] = pt;
	}

	/* Sort insertion points and remove duplicates */
	for (i = 0; i < ninserts; i++) {
		for (j = i + 1; j < ninserts; j++) {
			if (insert_points[j] < insert_points[i]) {
				size_t tmp = insert_points[i];
				insert_points[i] = insert_points[j];
				insert_points[j] = tmp;
			}
		}
	}

	/* Remove duplicates */
	j = 0;
	for (i = 0; i < ninserts; i++) {
		if (i == 0 || insert_points[i] != insert_points[j - 1])
			insert_points[j++] = insert_points[i];
	}
	ninserts = j;

	/* Allocate new lines array */
	newcount = staged->nlines + ninserts;
	newlines = malloc(newcount * sizeof(struct line));
	if (newlines == NULL) {
		free(insert_points);
		return -1;
	}

	/* Build new lines array with insertions */
	j = 0;
	r = 0;
	for (i = 0; i < staged->nlines; i++) {
		/* Check if we need to insert before this line */
		while (r < ninserts && insert_points[r] == i) {
			newlines[j].text = strdup(text);
			if (newlines[j].text == NULL) {
				/* Cleanup on error */
				while (j > 0) {
					j--;
					free(newlines[j].text);
				}
				free(newlines);
				free(insert_points);
				return -1;
			}
			newlines[j].len = strlen(text);
			j++;
			inserted++;
			r++;
		}
		newlines[j].text = staged->lines[i].text;
		newlines[j].len = staged->lines[i].len;
		j++;
	}

	/* Handle insertions at end of file */
	while (r < ninserts) {
		newlines[j].text = strdup(text);
		if (newlines[j].text == NULL) {
			while (j > 0) {
				j--;
				free(newlines[j].text);
			}
			free(newlines);
			free(insert_points);
			return -1;
		}
		newlines[j].len = strlen(text);
		j++;
		inserted++;
		r++;
	}

	free(insert_points);
	free(staged->lines);
	staged->lines = newlines;
	staged->nlines = newcount;
	staged->cap = newcount;

	return (int)inserted;
}

int
transform_move(struct buffer *staged, const struct selection *sel,
	       size_t target_line, int position)
{
	struct line *moved_lines;
	struct line *newlines;
	size_t i, j, k;
	size_t nmoved = 0;
	size_t remaining;
	size_t insert_pos;

	if (sel->nranges == 0)
		return 0;

	/* Count selected lines */
	for (i = 0; i < staged->nlines; i++) {
		if (is_line_selected(sel, i))
			nmoved++;
	}

	if (nmoved == 0)
		return 0;

	/* Collect moved lines */
	moved_lines = malloc(nmoved * sizeof(struct line));
	if (moved_lines == NULL)
		return -1;

	j = 0;
	for (i = 0; i < staged->nlines; i++) {
		if (is_line_selected(sel, i)) {
			moved_lines[j].text = staged->lines[i].text;
			moved_lines[j].len = staged->lines[i].len;
			j++;
		}
	}

	/* Build remaining lines (excluding selected) */
	remaining = staged->nlines - nmoved;
	newlines = malloc(staged->nlines * sizeof(struct line));
	if (newlines == NULL) {
		free(moved_lines);
		return -1;
	}

	/* Calculate insert position after removal */
	insert_pos = target_line;
	if (position == 1)
		insert_pos = target_line + 1;

	/* Adjust insert position for removed lines before target */
	size_t removed_before = 0;
	for (i = 0; i < staged->nlines && i < target_line; i++) {
		if (is_line_selected(sel, i))
			removed_before++;
	}
	if (insert_pos > removed_before)
		insert_pos -= removed_before;
	else
		insert_pos = 0;

	if (insert_pos > remaining)
		insert_pos = remaining;

	/* Build new array: remaining lines with moved lines inserted */
	j = 0;
	k = 0;
	for (i = 0; i < staged->nlines; i++) {
		if (!is_line_selected(sel, i)) {
			if (k == insert_pos) {
				/* Insert moved lines here */
				size_t m;
				for (m = 0; m < nmoved; m++) {
					newlines[j].text = moved_lines[m].text;
					newlines[j].len = moved_lines[m].len;
					j++;
				}
			}
			newlines[j].text = staged->lines[i].text;
			newlines[j].len = staged->lines[i].len;
			j++;
			k++;
		}
	}

	/* If insert position is at the end */
	if (insert_pos >= remaining) {
		size_t m;
		for (m = 0; m < nmoved; m++) {
			newlines[j].text = moved_lines[m].text;
			newlines[j].len = moved_lines[m].len;
			j++;
		}
	}

	free(moved_lines);
	free(staged->lines);
	staged->lines = newlines;
	staged->nlines = staged->nlines; /* unchanged */
	staged->cap = staged->nlines;

	return (int)nmoved;
}
