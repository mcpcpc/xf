#ifndef BUFFER_H
#define BUFFER_H

#include <stddef.h>

/* Line structure */
struct line {
	char *text;
	size_t len;
};

/* File boundary within a multi-file buffer */
struct file_region {
	char *filename;
	size_t start_line;  /* inclusive, 0-based */
	size_t end_line;    /* inclusive, 0-based */
};

/* Buffer holds the document as an array of lines */
struct buffer {
	struct line *lines;
	size_t nlines;
	size_t cap;
	char *filename;          /* primary filename (or NULL for multi) */
	struct file_region *regions;  /* file boundaries for multi-file */
	size_t nregions;
};

/* Load file into buffer, returns 0 on success, -1 on error */
int buffer_load(struct buffer *buf, const char *filename);

/* Load multiple files into buffer, concatenated with markers */
int buffer_load_multi(struct buffer *buf, const char **files, size_t nfiles);

/* Free buffer resources */
void buffer_free(struct buffer *buf);

/* Get line by index (0-based), returns NULL if out of range */
struct line *buffer_get_line(struct buffer *buf, size_t idx);

/* Replace buffer contents with new lines array */
int buffer_replace_lines(struct buffer *buf, struct line *lines, size_t nlines);

/* Duplicate buffer for staging */
int buffer_dup(struct buffer *dst, const struct buffer *src);

/* Write buffer back to file(s), handling multi-file regions */
int buffer_commit(struct buffer *staged, struct buffer *orig);

#endif /* BUFFER_H */
