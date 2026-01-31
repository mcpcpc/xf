#include "diff.h"
#include "buffer.h"

#include <stdio.h>
#include <string.h>

void
diff_print(const struct buffer *orig, const struct buffer *staged)
{
	size_t i, j;
	size_t max_lines;

	if (orig->filename != NULL)
		printf("--- %s\n+++ %s (staged)\n", orig->filename, orig->filename);

	max_lines = orig->nlines > staged->nlines ? orig->nlines : staged->nlines;

	i = 0;
	j = 0;

	while (i < orig->nlines || j < staged->nlines) {
		/* Both have lines at this position */
		if (i < orig->nlines && j < staged->nlines) {
			if (strcmp(orig->lines[i].text, staged->lines[j].text) != 0) {
				printf("@@ -%zu +%zu @@\n", i + 1, j + 1);
				printf("-%s\n", orig->lines[i].text);
				printf("+%s\n", staged->lines[j].text);
			}
			i++;
			j++;
		}
		/* Only original has line (deleted) */
		else if (i < orig->nlines) {
			printf("@@ -%zu @@\n", i + 1);
			printf("-%s\n", orig->lines[i].text);
			i++;
		}
		/* Only staged has line (added) */
		else {
			printf("@@ +%zu @@\n", j + 1);
			printf("+%s\n", staged->lines[j].text);
			j++;
		}
	}

	/* Suppress unused variable warning */
	(void)max_lines;
}
