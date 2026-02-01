#ifndef SELECTION_H
#define SELECTION_H

#include <stddef.h>

struct buffer;

/* A range of lines (inclusive, 0-based) */
struct range {
	size_t start;
	size_t end;
};

/* Selection is a set of ranges */
struct selection {
	struct range *ranges;
	size_t nranges;
	size_t cap;
	char name[32];
};

/* Named selection storage */
#define MAX_NAMED_SELECTIONS 16

struct selection_table {
	struct selection slots[MAX_NAMED_SELECTIONS];
	size_t count;
};

/* Initialize empty selection */
void selection_init(struct selection *sel);

/* Free selection resources */
void selection_free(struct selection *sel);

/* Clear selection */
void selection_clear(struct selection *sel);

/* Copy selection */
int selection_copy(struct selection *dst, const struct selection *src);

/* Set selection name */
void selection_set_name(struct selection *sel, const char *name);

/* Add a range to selection */
int selection_add_range(struct selection *sel, size_t start, size_t end);

/* Select all lines */
int selection_all(struct selection *sel, const struct buffer *buf);

/* Select lines matching substring */
int selection_lines_matching(struct selection *sel, const struct buffer *buf,
		const char *pattern);

/* Select lines matching regex */
int selection_lines_regex(struct selection *sel, const struct buffer *buf,
		const char *pattern);

/* Select paragraphs (blocks separated by blank lines) */
int selection_paragraphs(struct selection *sel, const struct buffer *buf);

/* Select paragraphs containing substring */
int selection_paragraphs_containing(struct selection *sel,
		const struct buffer *buf, const char *pattern);

/* Select paragraphs matching regex */
int selection_paragraphs_regex(struct selection *sel, const struct buffer *buf,
		const char *pattern);

/* Refine: keep only ranges containing pattern */
int selection_refine_containing(struct selection *sel, const struct buffer *buf,
		const char *pattern);

/* Refine: exclude ranges containing pattern */
int selection_refine_excluding(struct selection *sel, const struct buffer *buf,
		const char *pattern);

/* Refine: keep only ranges matching regex */
int selection_refine_regex(struct selection *sel, const struct buffer *buf,
		const char *pattern);

/* Refine: exclude ranges matching regex */
int selection_refine_excluding_regex(struct selection *sel,
		const struct buffer *buf,
		const char *pattern);

/* Get total line count in selection */
size_t selection_line_count(const struct selection *sel);

/* Selection table functions */
void selection_table_init(struct selection_table *tbl);
void selection_table_free(struct selection_table *tbl);
struct selection *selection_table_get(struct selection_table *tbl,
		const char *name);
int selection_table_store(struct selection_table *tbl, const char *name,
		const struct selection *sel);

/* Phase 2: Structural selections */

/* Select indentation-based blocks */
int selection_blocks(struct selection *sel, const struct buffer *buf);

/* Select blocks containing pattern */
int selection_blocks_containing(struct selection *sel, const struct buffer *buf,
		const char *pattern);

/* Select blocks matching regex */
int selection_blocks_regex(struct selection *sel, const struct buffer *buf,
		const char *pattern);

/* Select C-style functions (heuristic detection) */
int selection_functions(struct selection *sel, const struct buffer *buf);

/* Select functions containing pattern */
int selection_functions_containing(struct selection *sel,
		const struct buffer *buf, const char *pattern);

/* Check if line is a comment */
int line_is_comment(const char *text);

/* Refine: exclude comment lines */
int selection_refine_excluding_comments(struct selection *sel,
		const struct buffer *buf);

#endif /* SELECTION_H */
