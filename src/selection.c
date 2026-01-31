#include "selection.h"
#include "buffer.h"

#include <regex.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAP 16

void
selection_init(struct selection *sel)
{
	memset(sel, 0, sizeof(*sel));
}

void
selection_free(struct selection *sel)
{
	free(sel->ranges);
	memset(sel, 0, sizeof(*sel));
}

void
selection_clear(struct selection *sel)
{
	sel->nranges = 0;
}

int
selection_copy(struct selection *dst, const struct selection *src)
{
	selection_init(dst);

	if (src->nranges > 0) {
		dst->ranges = malloc(src->nranges * sizeof(struct range));
		if (dst->ranges == NULL)
			return -1;
		memcpy(dst->ranges, src->ranges,
		       src->nranges * sizeof(struct range));
		dst->nranges = src->nranges;
		dst->cap = src->nranges;
	}

	memcpy(dst->name, src->name, sizeof(dst->name));
	return 0;
}

void
selection_set_name(struct selection *sel, const char *name)
{
	if (name == NULL) {
		sel->name[0] = '\0';
		return;
	}
	strncpy(sel->name, name, sizeof(sel->name) - 1);
	sel->name[sizeof(sel->name) - 1] = '\0';
}

int
selection_add_range(struct selection *sel, size_t start, size_t end)
{
	if (sel->ranges == NULL) {
		sel->cap = INITIAL_CAP;
		sel->ranges = malloc(sel->cap * sizeof(struct range));
		if (sel->ranges == NULL)
			return -1;
	}

	if (sel->nranges >= sel->cap) {
		size_t newcap = sel->cap * 2;
		struct range *tmp;
		tmp = realloc(sel->ranges, newcap * sizeof(struct range));
		if (tmp == NULL)
			return -1;
		sel->ranges = tmp;
		sel->cap = newcap;
	}

	sel->ranges[sel->nranges].start = start;
	sel->ranges[sel->nranges].end = end;
	sel->nranges++;

	return 0;
}

int
selection_all(struct selection *sel, const struct buffer *buf)
{
	selection_clear(sel);
	if (buf->nlines == 0)
		return 0;
	return selection_add_range(sel, 0, buf->nlines - 1);
}

int
selection_lines_matching(struct selection *sel, const struct buffer *buf,
			 const char *pattern)
{
	size_t i;

	selection_clear(sel);

	for (i = 0; i < buf->nlines; i++) {
		if (strstr(buf->lines[i].text, pattern) != NULL) {
			if (selection_add_range(sel, i, i) < 0)
				return -1;
		}
	}

	return 0;
}

int
selection_lines_regex(struct selection *sel, const struct buffer *buf,
		      const char *pattern)
{
	regex_t re;
	size_t i;
	int rc;

	selection_clear(sel);

	rc = regcomp(&re, pattern, REG_EXTENDED | REG_NOSUB);
	if (rc != 0)
		return -1;

	for (i = 0; i < buf->nlines; i++) {
		if (regexec(&re, buf->lines[i].text, 0, NULL, 0) == 0) {
			if (selection_add_range(sel, i, i) < 0) {
				regfree(&re);
				return -1;
			}
		}
	}

	regfree(&re);
	return 0;
}

int
selection_paragraphs(struct selection *sel, const struct buffer *buf)
{
	size_t i, start;
	int in_para = 0;

	selection_clear(sel);

	for (i = 0; i < buf->nlines; i++) {
		int is_blank = (buf->lines[i].len == 0);

		if (!in_para && !is_blank) {
			start = i;
			in_para = 1;
		} else if (in_para && is_blank) {
			if (selection_add_range(sel, start, i - 1) < 0)
				return -1;
			in_para = 0;
		}
	}

	if (in_para) {
		if (selection_add_range(sel, start, buf->nlines - 1) < 0)
			return -1;
	}

	return 0;
}

int
selection_paragraphs_containing(struct selection *sel, const struct buffer *buf,
				const char *pattern)
{
	size_t i, start;
	int in_para = 0;
	int has_match = 0;

	selection_clear(sel);

	for (i = 0; i < buf->nlines; i++) {
		int is_blank = (buf->lines[i].len == 0);

		if (!in_para && !is_blank) {
			start = i;
			in_para = 1;
			has_match = 0;
		}

		if (in_para && !is_blank) {
			if (strstr(buf->lines[i].text, pattern) != NULL)
				has_match = 1;
		}

		if (in_para && is_blank) {
			if (has_match) {
				if (selection_add_range(sel, start, i - 1) < 0)
					return -1;
			}
			in_para = 0;
		}
	}

	if (in_para && has_match) {
		if (selection_add_range(sel, start, buf->nlines - 1) < 0)
			return -1;
	}

	return 0;
}

int
selection_paragraphs_regex(struct selection *sel, const struct buffer *buf,
			   const char *pattern)
{
	regex_t re;
	size_t i, start;
	int in_para = 0;
	int has_match = 0;
	int rc;

	selection_clear(sel);

	rc = regcomp(&re, pattern, REG_EXTENDED | REG_NOSUB);
	if (rc != 0)
		return -1;

	for (i = 0; i < buf->nlines; i++) {
		int is_blank = (buf->lines[i].len == 0);

		if (!in_para && !is_blank) {
			start = i;
			in_para = 1;
			has_match = 0;
		}

		if (in_para && !is_blank) {
			if (regexec(&re, buf->lines[i].text, 0, NULL, 0) == 0)
				has_match = 1;
		}

		if (in_para && is_blank) {
			if (has_match) {
				if (selection_add_range(sel, start, i - 1) < 0) {
					regfree(&re);
					return -1;
				}
			}
			in_para = 0;
		}
	}

	if (in_para && has_match) {
		if (selection_add_range(sel, start, buf->nlines - 1) < 0) {
			regfree(&re);
			return -1;
		}
	}

	regfree(&re);
	return 0;
}

static int
range_contains_pattern(const struct buffer *buf, const struct range *r,
		const char *pattern)
{
	size_t i;

	for (i = r->start; i <= r->end; i++) {
		if (strstr(buf->lines[i].text, pattern) != NULL)
			return 1;
	}
	return 0;
}

static int
range_matches_regex(const struct buffer *buf, const struct range *r,
		regex_t *re)
{
	size_t i;

	for (i = r->start; i <= r->end; i++) {
		if (regexec(re, buf->lines[i].text, 0, NULL, 0) == 0)
			return 1;
	}
	return 0;
}

int
selection_refine_containing(struct selection *sel, const struct buffer *buf,
			const char *pattern)
{
	size_t i, j;

	j = 0;
	for (i = 0; i < sel->nranges; i++) {
		if (range_contains_pattern(buf, &sel->ranges[i], pattern)) {
			sel->ranges[j++] = sel->ranges[i];
		}
	}
	sel->nranges = j;

	return 0;
}

int
selection_refine_excluding(struct selection *sel, const struct buffer *buf,
			const char *pattern)
{
	size_t i, j;

	j = 0;
	for (i = 0; i < sel->nranges; i++) {
		if (!range_contains_pattern(buf, &sel->ranges[i], pattern)) {
			sel->ranges[j++] = sel->ranges[i];
		}
	}
	sel->nranges = j;

	return 0;
}

int
selection_refine_regex(struct selection *sel, const struct buffer *buf,
		const char *pattern)
{
	regex_t re;
	size_t i, j;
	int rc;

	rc = regcomp(&re, pattern, REG_EXTENDED | REG_NOSUB);
	if (rc != 0)
		return -1;

	j = 0;
	for (i = 0; i < sel->nranges; i++) {
		if (range_matches_regex(buf, &sel->ranges[i], &re)) {
			sel->ranges[j++] = sel->ranges[i];
		}
	}
	sel->nranges = j;

	regfree(&re);
	return 0;
}

int
selection_refine_excluding_regex(struct selection *sel, const struct buffer *buf,
		const char *pattern)
{
	regex_t re;
	size_t i, j;
	int rc;

	rc = regcomp(&re, pattern, REG_EXTENDED | REG_NOSUB);
	if (rc != 0)
		return -1;

	j = 0;
	for (i = 0; i < sel->nranges; i++) {
		if (!range_matches_regex(buf, &sel->ranges[i], &re)) {
			sel->ranges[j++] = sel->ranges[i];
		}
	}
	sel->nranges = j;

	regfree(&re);
	return 0;
}

size_t
selection_line_count(const struct selection *sel)
{
	size_t i, count = 0;

	for (i = 0; i < sel->nranges; i++)
		count += sel->ranges[i].end - sel->ranges[i].start + 1;

	return count;
}

/* Selection table functions */

void
selection_table_init(struct selection_table *tbl)
{
	size_t i;

	tbl->count = 0;
	for (i = 0; i < MAX_NAMED_SELECTIONS; i++)
		selection_init(&tbl->slots[i]);
}

void
selection_table_free(struct selection_table *tbl)
{
	size_t i;

	for (i = 0; i < MAX_NAMED_SELECTIONS; i++)
		selection_free(&tbl->slots[i]);
	tbl->count = 0;
}

struct selection *
selection_table_get(struct selection_table *tbl, const char *name)
{
	size_t i;

	for (i = 0; i < tbl->count; i++) {
		if (strcmp(tbl->slots[i].name, name) == 0)
			return &tbl->slots[i];
	}
	return NULL;
}

int
selection_table_store(struct selection_table *tbl, const char *name,
		const struct selection *sel)
{
	struct selection *existing;
	size_t slot;

	existing = selection_table_get(tbl, name);
	if (existing != NULL) {
		selection_free(existing);
		return selection_copy(existing, sel);
	}

	if (tbl->count >= MAX_NAMED_SELECTIONS)
		return -1;

	slot = tbl->count++;
	if (selection_copy(&tbl->slots[slot], sel) < 0)
		return -1;

	selection_set_name(&tbl->slots[slot], name);
	return 0;
}

/* Get indentation level (number of leading whitespace chars) */
static size_t
get_indent(const char *text)
{
	size_t indent = 0;

	while (*text == ' ' || *text == '\t') {
		indent++;
		text++;
	}
	return indent;
}

/* Check if line is blank or whitespace-only */
static int
is_blank_line(const char *text)
{
	while (*text) {
		if (*text != ' ' && *text != '\t')
			return 0;
		text++;
	}
	return 1;
}

int
selection_blocks(struct selection *sel, const struct buffer *buf)
{
	size_t i, start;
	size_t base_indent;
	int in_block = 0;

	selection_clear(sel);

	for (i = 0; i < buf->nlines; i++) {
		const char *text = buf->lines[i].text;

		if (is_blank_line(text)) {
			if (in_block) {
				if (selection_add_range(sel, start, i - 1) < 0)
					return -1;
				in_block = 0;
			}
			continue;
		}

		if (!in_block) {
			start = i;
			base_indent = get_indent(text);
			in_block = 1;
		} else {
			size_t cur_indent = get_indent(text);
			/* New block starts at same or lower indent */
			if (cur_indent <= base_indent && cur_indent == 0) {
				if (selection_add_range(sel, start, i - 1) < 0)
					return -1;
				start = i;
				base_indent = cur_indent;
			}
		}
	}

	if (in_block) {
		if (selection_add_range(sel, start, buf->nlines - 1) < 0)
			return -1;
	}

	return 0;
}

int
selection_blocks_containing(struct selection *sel, const struct buffer *buf,
		const char *pattern)
{
	if (selection_blocks(sel, buf) < 0)
		return -1;
	return selection_refine_containing(sel, buf, pattern);
}

int
selection_blocks_regex(struct selection *sel, const struct buffer *buf,
		const char *pattern)
{
	if (selection_blocks(sel, buf) < 0)
		return -1;
	return selection_refine_regex(sel, buf, pattern);
}

/* Check if line looks like a function definition start */
static int
is_function_start(const char *text)
{
	const char *p;
	int has_paren = 0;
	int has_brace = 0;

	/* Skip leading whitespace */
	while (*text == ' ' || *text == '\t')
		text++;

	/* Skip if starts with # (preprocessor) or // (comment) */
	if (*text == '#' || (text[0] == '/' && text[1] == '/'))
		return 0;

	/* Look for pattern: something followed by (...) and possibly { */
	p = text;
	while (*p) {
		if (*p == '(')
			has_paren = 1;
		if (*p == ')' && has_paren)
			has_paren = 2;
		if (*p == '{' && has_paren == 2)
			has_brace = 1;
		p++;
	}

	/* Function if we have () and ends with { or just ) */
	if (has_paren == 2 && has_brace)
		return 1;

	/* Check for K&R style: ) at end of line */
	p = text + strlen(text) - 1;
	while (p > text && (*p == ' ' || *p == '\t'))
		p--;
	if (*p == ')' && has_paren == 2)
		return 1;

	return 0;
}

/* Find matching closing brace, counting nesting */
static size_t
find_function_end(const struct buffer *buf, size_t start)
{
	size_t i;
	int brace_count = 0;
	int found_open = 0;

	for (i = start; i < buf->nlines; i++) {
		const char *p = buf->lines[i].text;

		while (*p) {
			if (*p == '{') {
				brace_count++;
				found_open = 1;
			} else if (*p == '}') {
				brace_count--;
				if (found_open && brace_count == 0)
					return i;
			}
			p++;
		}
	}

	return buf->nlines - 1;
}

int
selection_functions(struct selection *sel, const struct buffer *buf)
{
	size_t i;

	selection_clear(sel);

	for (i = 0; i < buf->nlines; i++) {
		if (is_function_start(buf->lines[i].text)) {
			size_t end = find_function_end(buf, i);
			if (selection_add_range(sel, i, end) < 0)
				return -1;
			i = end;  /* Skip past this function */
		}
	}

	return 0;
}

int
selection_functions_containing(struct selection *sel, const struct buffer *buf,
		const char *pattern)
{
	if (selection_functions(sel, buf) < 0)
		return -1;
	return selection_refine_containing(sel, buf, pattern);
}

int
line_is_comment(const char *text)
{
	/* Skip leading whitespace */
	while (*text == ' ' || *text == '\t')
		text++;

	/* C++ style comment */
	if (text[0] == '/' && text[1] == '/')
		return 1;

	/* C style comment start (simplified: just checks line start) */
	if (text[0] == '/' && text[1] == '*')
		return 1;

	/* Line starting with * (inside block comment) */
	if (text[0] == '*')
		return 1;

	return 0;
}

int
selection_refine_excluding_comments(struct selection *sel,
		const struct buffer *buf)
{
	size_t i, j;

	j = 0;
	for (i = 0; i < sel->nranges; i++) {
		struct range *r = &sel->ranges[i];
		size_t k;
		int all_comments = 1;

		for (k = r->start; k <= r->end; k++) {
			if (!line_is_comment(buf->lines[k].text)) {
				all_comments = 0;
				break;
			}
		}

		if (!all_comments) {
			sel->ranges[j++] = sel->ranges[i];
		}
	}
	sel->nranges = j;

	return 0;
}
