#ifndef DIFF_H
#define DIFF_H

struct buffer;

/* Print unified diff between original and staged buffer */
void diff_print(const struct buffer *orig, const struct buffer *staged);

#endif /* DIFF_H */
