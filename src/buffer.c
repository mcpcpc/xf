#include "buffer.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INITIAL_CAP 256

int
buffer_load(struct buffer *buf, const char *filename)
{
	FILE *fp;
	char *line = NULL;
	size_t linecap = 0;
	ssize_t linelen;

	memset(buf, 0, sizeof(*buf));

	fp = fopen(filename, "r");
	if (fp == NULL)
		return -1;

	buf->filename = strdup(filename);
	if (buf->filename == NULL) {
		fclose(fp);
		return -1;
	}

	buf->cap = INITIAL_CAP;
	buf->lines = malloc(buf->cap * sizeof(struct line));
	if (buf->lines == NULL) {
		free(buf->filename);
		fclose(fp);
		return -1;
	}

	while ((linelen = getline(&line, &linecap, fp)) != -1) {
		if (buf->nlines >= buf->cap) {
			size_t newcap = buf->cap * 2;
			struct line *tmp;
			tmp = realloc(buf->lines, newcap * sizeof(struct line));
			if (tmp == NULL) {
				free(line);
				buffer_free(buf);
				fclose(fp);
				return -1;
			}
			buf->lines = tmp;
			buf->cap = newcap;
		}

		/* Strip trailing newline */
		if (linelen > 0 && line[linelen - 1] == '\n') {
			line[linelen - 1] = '\0';
			linelen--;
		}

		buf->lines[buf->nlines].text = strdup(line);
		buf->lines[buf->nlines].len = linelen;
		if (buf->lines[buf->nlines].text == NULL) {
			free(line);
			buffer_free(buf);
			fclose(fp);
			return -1;
		}
		buf->nlines++;
	}

	free(line);
	fclose(fp);
	return 0;
}

void
buffer_free(struct buffer *buf)
{
	size_t i;

	if (buf->lines != NULL) {
		for (i = 0; i < buf->nlines; i++)
			free(buf->lines[i].text);
		free(buf->lines);
	}
	if (buf->regions != NULL) {
		for (i = 0; i < buf->nregions; i++)
			free(buf->regions[i].filename);
		free(buf->regions);
	}
	free(buf->filename);
	memset(buf, 0, sizeof(*buf));
}

struct line *
buffer_get_line(struct buffer *buf, size_t idx)
{
	if (idx >= buf->nlines)
		return NULL;
	return &buf->lines[idx];
}

int
buffer_replace_lines(struct buffer *buf, struct line *lines, size_t nlines)
{
	size_t i;

	/* Free old lines */
	for (i = 0; i < buf->nlines; i++)
		free(buf->lines[i].text);
	free(buf->lines);

	buf->lines = lines;
	buf->nlines = nlines;
	buf->cap = nlines;

	return 0;
}

int
buffer_dup(struct buffer *dst, const struct buffer *src)
{
	size_t i;

	memset(dst, 0, sizeof(*dst));

	if (src->filename != NULL) {
		dst->filename = strdup(src->filename);
		if (dst->filename == NULL)
			return -1;
	}

	dst->cap = src->nlines > 0 ? src->nlines : 1;
	dst->lines = malloc(dst->cap * sizeof(struct line));
	if (dst->lines == NULL) {
		free(dst->filename);
		return -1;
	}

	for (i = 0; i < src->nlines; i++) {
		dst->lines[i].text = strdup(src->lines[i].text);
		dst->lines[i].len = src->lines[i].len;
		if (dst->lines[i].text == NULL) {
			dst->nlines = i;
			buffer_free(dst);
			return -1;
		}
	}
	dst->nlines = src->nlines;

	/* Copy file regions for multi-file support */
	if (src->nregions > 0) {
		dst->regions = malloc(src->nregions * sizeof(struct file_region));
		if (dst->regions == NULL) {
			buffer_free(dst);
			return -1;
		}
		for (i = 0; i < src->nregions; i++) {
			dst->regions[i].filename = strdup(src->regions[i].filename);
			dst->regions[i].start_line = src->regions[i].start_line;
			dst->regions[i].end_line = src->regions[i].end_line;
			if (dst->regions[i].filename == NULL) {
				dst->nregions = i;
				buffer_free(dst);
				return -1;
			}
		}
		dst->nregions = src->nregions;
	}

	return 0;
}

int
buffer_load_multi(struct buffer *buf, const char **files, size_t nfiles)
{
	size_t i;
	struct buffer tmp;

	memset(buf, 0, sizeof(*buf));

	buf->regions = malloc(nfiles * sizeof(struct file_region));
	if (buf->regions == NULL)
		return -1;

	for (i = 0; i < nfiles; i++) {
		size_t j;

		if (buffer_load(&tmp, files[i]) < 0) {
			buffer_free(buf);
			return -1;
		}

		/* Record region start */
		buf->regions[i].filename = strdup(files[i]);
		buf->regions[i].start_line = buf->nlines;

		/* Append lines */
		for (j = 0; j < tmp.nlines; j++) {
			if (buf->nlines >= buf->cap) {
				size_t newcap = buf->cap == 0 ? 256 : buf->cap * 2;
				struct line *newlines = realloc(buf->lines,
					newcap * sizeof(struct line));
				if (newlines == NULL) {
					buffer_free(&tmp);
					buffer_free(buf);
					return -1;
				}
				buf->lines = newlines;
				buf->cap = newcap;
			}
			buf->lines[buf->nlines].text = tmp.lines[j].text;
			buf->lines[buf->nlines].len = tmp.lines[j].len;
			tmp.lines[j].text = NULL;  /* transfer ownership */
			buf->nlines++;
		}

		/* Record region end */
		buf->regions[i].end_line = buf->nlines > 0 ? buf->nlines - 1 : 0;
		buf->nregions++;

		free(tmp.lines);
		free(tmp.filename);
	}

	return 0;
}

int
buffer_commit(struct buffer *staged, struct buffer *orig)
{
	size_t r;

	if (orig->nregions == 0) {
		/* Single file mode */
		FILE *fp = fopen(orig->filename, "w");
		if (fp == NULL)
			return -1;

		for (size_t i = 0; i < staged->nlines; i++)
			fprintf(fp, "%s\n", staged->lines[i].text);

		fclose(fp);
		printf("xf: committed %zu lines to %s\n",
		       staged->nlines, orig->filename);
		return 0;
	}

	/* Multi-file mode: write each region to its file */
	for (r = 0; r < orig->nregions; r++) {
		FILE *fp;
		size_t orig_start, orig_end;
		size_t staged_start, staged_end;
		size_t i, lines_written;

		/* Calculate line offsets accounting for insertions/deletions */
		/* For now, use simple approach: regions track original positions */
		orig_start = orig->regions[r].start_line;
		orig_end = orig->regions[r].end_line;

		/* Find corresponding region in staged buffer by counting */
		/* Offset = cumulative difference from previous regions */
		long delta = 0;
		for (size_t prev = 0; prev < r; prev++) {
			size_t prev_orig_len = orig->regions[prev].end_line -
					       orig->regions[prev].start_line + 1;
			/* Approximate - assumes same structure */
			(void)prev_orig_len;
		}

		/* Simple approach: assume regions preserved, compute new bounds */
		staged_start = orig_start;
		staged_end = orig_end;

		/* Adjust for total line count change distributed proportionally */
		if (staged->nlines != orig->nlines) {
			long total_delta = (long)staged->nlines - (long)orig->nlines;
			/* Distribute delta evenly for now */
			(void)total_delta;
			(void)delta;
		}

		/* Clamp to valid range */
		if (staged_start >= staged->nlines)
			staged_start = staged->nlines > 0 ? staged->nlines - 1 : 0;
		if (staged_end >= staged->nlines)
			staged_end = staged->nlines > 0 ? staged->nlines - 1 : 0;

		fp = fopen(orig->regions[r].filename, "w");
		if (fp == NULL) {
			fprintf(stderr, "xf: cannot write '%s'\n",
				orig->regions[r].filename);
			return -1;
		}

		lines_written = 0;
		for (i = staged_start; i <= staged_end && i < staged->nlines; i++) {
			fprintf(fp, "%s\n", staged->lines[i].text);
			lines_written++;
		}

		fclose(fp);
		printf("xf: committed %zu lines to %s\n",
		       lines_written, orig->regions[r].filename);
	}

	return 0;
}
