#include "buffer.h"
#include "command.h"
#include "diff.h"
#include "history.h"
#include "selection.h"
#include "transform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void
print_help(void)
{
	printf("xf commands:\n");
	printf("  select lines matching \"pattern\"        Select lines containing pattern\n");
	printf("  select lines matching /regex/          Select lines matching regex\n");
	printf("  select paragraphs                      Select all paragraphs\n");
	printf("  select paragraphs containing \"pat\"    Select paragraphs with pattern\n");
	printf("  select blocks                          Select indentation-based blocks\n");
	printf("  select functions                       Select C-style functions\n");
	printf("  select functions containing \"pat\"     Select functions with pattern\n");
	printf("  select ... as <name>                   Name the selection\n");
	printf("  refine containing \"pattern\"           Keep selections with pattern\n");
	printf("  refine excluding \"pattern\"            Remove selections with pattern\n");
	printf("  refine containing /regex/              Keep selections matching regex\n");
	printf("  refine excluding comments              Remove comment-only selections\n");
	printf("  replace \"old\" with \"new\"              Replace in selected lines\n");
	printf("  delete                                 Delete selected lines\n");
	printf("  insert \"text\" before                   Insert line before selection\n");
	printf("  insert \"text\" after                    Insert line after selection\n");
	printf("  move before <line>                     Move selection before line N\n");
	printf("  move after <line>                      Move selection after line N\n");
	printf("  preview                                Show staged changes\n");
	printf("  commit                                 Apply staged changes\n");
	printf("  abort                                  Discard staged changes\n");
	printf("  selections                             List named selections\n");
	printf("  history                                Show command history\n");
	printf("  save <file>                            Save command history to file\n");
	printf("  replay <file>                          Execute commands from file\n");
	printf("  quit                                   Exit xf\n");
}

static void
print_selection(const struct selection *sel, const struct buffer *buf)
{
	size_t i, j;
	size_t lines_shown = 0;
	size_t max_preview = 5;

	if (sel->name[0] != '\0')
		printf("xf: [%s] ", sel->name);
	else
		printf("xf: ");

	printf("%zu range(s), %zu line(s) selected\n",
	       sel->nranges, selection_line_count(sel));

	for (i = 0; i < sel->nranges && lines_shown < max_preview; i++) {
		for (j = sel->ranges[i].start;
		     j <= sel->ranges[i].end && lines_shown < max_preview;
		     j++) {
			printf("  %4zu: | %s\n", j + 1, buf->lines[j].text);
			lines_shown++;
		}
	}

	if (selection_line_count(sel) > max_preview)
		printf("  ... (%zu more lines)\n",
		       selection_line_count(sel) - max_preview);
}

static void
print_selections(const struct selection_table *tbl)
{
	size_t i;

	if (tbl->count == 0) {
		printf("xf: no named selections\n");
		return;
	}

	printf("xf: %zu named selection(s)\n", tbl->count);
	for (i = 0; i < tbl->count; i++) {
		printf("  [%s] %zu range(s), %zu line(s)\n",
			tbl->slots[i].name,
			tbl->slots[i].nranges,
			selection_line_count(&tbl->slots[i]));
	}
}

static void
print_history(const struct history *h)
{
	size_t i;

	if (h->count == 0) {
		printf("xf: no commands in history\n");
		return;
	}

	printf("xf: %zu command(s) in history\n", h->count);
	for (i = 0; i < h->count; i++)
		printf("  %3zu: %s\n", i + 1, h->entries[i]);
}

static int
handle_select(struct command *cmd, struct selection *sel,
	      struct selection_table *tbl, struct buffer *buf)
{
	int rc = 0;

	if (cmd->arg1 == NULL) {
		fprintf(stderr, "xf: select requires a type (lines, paragraphs, blocks, functions)\n");
		return -1;
	}

	if (strcmp(cmd->arg1, "lines") == 0) {
		if (cmd->arg2 != NULL && strcmp(cmd->arg2, "matching") == 0) {
			if (cmd->arg3 == NULL) {
				fprintf(stderr, "xf: select lines matching requires a pattern\n");
				return -1;
			}
			if (cmd->pat_type == PAT_REGEX)
				rc = selection_lines_regex(sel, buf, cmd->arg3);
			else
				rc = selection_lines_matching(sel, buf, cmd->arg3);
		} else {
			rc = selection_all(sel, buf);
		}
	} else if (strcmp(cmd->arg1, "paragraphs") == 0) {
		if (cmd->arg2 != NULL && strcmp(cmd->arg2, "containing") == 0) {
			if (cmd->arg3 == NULL) {
				fprintf(stderr, "xf: select paragraphs containing requires a pattern\n");
				return -1;
			}
			if (cmd->pat_type == PAT_REGEX)
				rc = selection_paragraphs_regex(sel, buf, cmd->arg3);
			else
				rc = selection_paragraphs_containing(sel, buf, cmd->arg3);
		} else {
			rc = selection_paragraphs(sel, buf);
		}
	} else if (strcmp(cmd->arg1, "blocks") == 0) {
		if (cmd->arg2 != NULL && strcmp(cmd->arg2, "containing") == 0) {
			if (cmd->arg3 == NULL) {
				fprintf(stderr, "xf: select blocks containing requires a pattern\n");
				return -1;
			}
			if (cmd->pat_type == PAT_REGEX)
				rc = selection_blocks_regex(sel, buf, cmd->arg3);
			else
				rc = selection_blocks_containing(sel, buf, cmd->arg3);
		} else {
			rc = selection_blocks(sel, buf);
		}
	} else if (strcmp(cmd->arg1, "functions") == 0) {
		if (cmd->arg2 != NULL && strcmp(cmd->arg2, "containing") == 0) {
			if (cmd->arg3 == NULL) {
				fprintf(stderr, "xf: select functions containing requires a pattern\n");
				return -1;
			}
			rc = selection_functions(sel, buf);
			if (rc >= 0)
				rc = selection_refine_containing(sel, buf, cmd->arg3);
		} else {
			rc = selection_functions(sel, buf);
		}
	} else if (strcmp(cmd->arg1, "all") == 0) {
		rc = selection_all(sel, buf);
	} else {
		fprintf(stderr, "xf: unknown selection type '%s'\n", cmd->arg1);
		return -1;
	}

	if (rc < 0) {
		fprintf(stderr, "xf: invalid regex pattern\n");
		return -1;
	}

	/* Handle naming */
	if (cmd->sel_name != NULL) {
		selection_set_name(sel, cmd->sel_name);
		if (selection_table_store(tbl, cmd->sel_name, sel) < 0) {
			fprintf(stderr, "xf: too many named selections\n");
		}
	}

	print_selection(sel, buf);
	return 0;
}

static int
handle_refine(struct command *cmd, struct selection *sel, struct buffer *buf)
{
	int rc = 0;

	if (cmd->arg1 == NULL) {
		fprintf(stderr, "xf: refine requires containing or excluding\n");
		return -1;
	}

	if (strcmp(cmd->arg1, "containing") == 0) {
		if (cmd->arg2 == NULL) {
			fprintf(stderr, "xf: refine containing requires a pattern\n");
			return -1;
		}
		if (cmd->pat_type == PAT_REGEX)
			rc = selection_refine_regex(sel, buf, cmd->arg2);
		else
			rc = selection_refine_containing(sel, buf, cmd->arg2);
	} else if (strcmp(cmd->arg1, "excluding") == 0) {
		/* Check for "excluding comments" */
		if (cmd->arg2 != NULL && strcmp(cmd->arg2, "comments") == 0) {
			rc = selection_refine_excluding_comments(sel, buf);
		} else if (cmd->arg2 == NULL) {
			fprintf(stderr, "xf: refine excluding requires a pattern\n");
			return -1;
		} else {
			if (cmd->pat_type == PAT_REGEX)
				rc = selection_refine_excluding_regex(sel, buf, cmd->arg2);
			else
				rc = selection_refine_excluding(sel, buf, cmd->arg2);
		}
	} else {
		fprintf(stderr, "xf: unknown refine type '%s'\n", cmd->arg1);
		return -1;
	}

	if (rc < 0) {
		fprintf(stderr, "xf: invalid regex pattern\n");
		return -1;
	}

	print_selection(sel, buf);
	return 0;
}

/* Editor state */
struct editor_state {
	struct buffer orig;
	struct buffer staged;
	struct selection sel;
	struct selection_table seltbl;
	struct history hist;
	int has_staged;
	int running;
};

/* Forward declaration */
static int execute_command(struct editor_state *st, const char *line,
		int log_cmd);

static int
replay_file(struct editor_state *st, const char *path)
{
	struct history replay;
	size_t i;

	history_init(&replay);
	if (history_load(&replay, path) < 0) {
		fprintf(stderr, "xf: cannot load '%s'\n", path);
		return -1;
	}

	printf("xf: replaying %zu command(s) from %s\n", replay.count, path);
	for (i = 0; i < replay.count && st->running; i++) {
		printf("> %s\n", replay.entries[i]);
		execute_command(st, replay.entries[i], 1);
	}

	history_free(&replay);
	return 0;
}

static int
execute_command(struct editor_state *st, const char *line, int log_cmd)
{
	struct command cmd;
	int should_log = 0;

	cmd = command_parse(line);

	switch (cmd.type) {
	case CMD_NONE:
		break;

	case CMD_SELECT:
		if (handle_select(&cmd, &st->sel, &st->seltbl, &st->staged) == 0)
			should_log = 1;
		break;

	case CMD_REFINE:
		if (handle_refine(&cmd, &st->sel, &st->staged) == 0)
			should_log = 1;
		break;

	case CMD_REPLACE:
		if (st->sel.nranges == 0) {
			fprintf(stderr, "xf: no selection\n");
		} else if (cmd.arg1 == NULL || cmd.arg2 == NULL) {
			fprintf(stderr, "xf: replace requires \"old\" with \"new\"\n");
		} else {
			int n = transform_replace(&st->staged, &st->sel,
						  cmd.arg1, cmd.arg2);
			if (n >= 0) {
				printf("xf: %d replacement(s) staged\n", n);
				st->has_staged = 1;
				should_log = 1;
			}
		}
		break;

	case CMD_DELETE:
		if (st->sel.nranges == 0) {
			fprintf(stderr, "xf: no selection\n");
		} else {
			int n = transform_delete(&st->staged, &st->sel);
			if (n >= 0) {
				printf("xf: %d line(s) deleted\n", n);
				st->has_staged = 1;
				selection_clear(&st->sel);
				should_log = 1;
			}
		}
		break;

	case CMD_INSERT:
		if (st->sel.nranges == 0) {
			fprintf(stderr, "xf: no selection\n");
		} else if (cmd.arg1 == NULL) {
			fprintf(stderr, "xf: insert requires \"text\" before|after\n");
		} else {
			int pos = 1;  /* default: after */
			if (cmd.arg2 != NULL && strcmp(cmd.arg2, "before") == 0)
				pos = 0;
			int n = transform_insert(&st->staged, &st->sel,
						 cmd.arg1, pos);
			if (n >= 0) {
				printf("xf: %d line(s) inserted\n", n);
				st->has_staged = 1;
				selection_clear(&st->sel);
				should_log = 1;
			}
		}
		break;

	case CMD_MOVE:
		if (st->sel.nranges == 0) {
			fprintf(stderr, "xf: no selection\n");
		} else if (cmd.arg1 == NULL || cmd.arg2 == NULL) {
			fprintf(stderr, "xf: move requires before|after <line>\n");
		} else {
			int pos = 0;  /* default: before */
			size_t target;
			if (strcmp(cmd.arg1, "after") == 0)
				pos = 1;
			target = (size_t)atol(cmd.arg2);
			if (target > 0)
				target--;  /* convert to 0-based */
			int n = transform_move(&st->staged, &st->sel,
					       target, pos);
			if (n >= 0) {
				printf("xf: %d line(s) moved\n", n);
				st->has_staged = 1;
				selection_clear(&st->sel);
				should_log = 1;
			}
		}
		break;

	case CMD_PREVIEW:
		if (!st->has_staged) {
			printf("xf: no staged changes\n");
		} else {
			diff_print(&st->orig, &st->staged);
		}
		break;

	case CMD_COMMIT:
		if (!st->has_staged) {
			printf("xf: no staged changes\n");
		} else {
			if (buffer_commit(&st->staged, &st->orig) < 0) {
				fprintf(stderr, "xf: commit failed\n");
			} else {
				buffer_free(&st->orig);
				buffer_dup(&st->orig, &st->staged);
				st->has_staged = 0;
				should_log = 1;
			}
		}
		break;

	case CMD_ABORT:
		buffer_free(&st->staged);
		buffer_dup(&st->staged, &st->orig);
		selection_clear(&st->sel);
		st->has_staged = 0;
		printf("xf: changes discarded\n");
		should_log = 1;
		break;

	case CMD_QUIT:
		st->running = 0;
		break;

	case CMD_HELP:
		print_help();
		break;

	case CMD_SELECTIONS:
		print_selections(&st->seltbl);
		break;

	case CMD_HISTORY:
		print_history(&st->hist);
		break;

	case CMD_SAVE:
		if (cmd.arg1 == NULL) {
			fprintf(stderr, "xf: save requires a filename\n");
		} else if (history_save(&st->hist, cmd.arg1) < 0) {
			fprintf(stderr, "xf: cannot save to '%s'\n", cmd.arg1);
		} else {
			printf("xf: saved %zu command(s) to %s\n",
			       st->hist.count, cmd.arg1);
		}
		break;

	case CMD_REPLAY:
		if (cmd.arg1 == NULL) {
			fprintf(stderr, "xf: replay requires a filename\n");
		} else {
			replay_file(st, cmd.arg1);
		}
		break;

	case CMD_UNKNOWN:
		fprintf(stderr, "xf: unknown command (try 'help')\n");
		break;
	}

	if (log_cmd && should_log)
		history_log(&st->hist, line);

	command_free(&cmd);
	return 0;
}

static void
print_usage(void)
{
	fprintf(stderr, "usage: xf [options] <file> [file ...]\n");
	fprintf(stderr, "options:\n");
	fprintf(stderr, "  --replay <file>  Replay commands then interactive\n");
	fprintf(stderr, "  --batch <file>   Execute commands and exit\n");
}

static int
process_files(const char **files, size_t nfiles, const char *replay_path,
		const char *batch_path)
{
	struct editor_state st;
	char *line = NULL;
	size_t linecap = 0;
	size_t i;

	memset(&st, 0, sizeof(st));
	st.running = 1;

	if (nfiles == 1) {
		/* Single file mode */
		if (buffer_load(&st.orig, files[0]) < 0) {
			fprintf(stderr, "xf: cannot load '%s'\n", files[0]);
			return 1;
		}
		printf("xf: loaded %s (%zu lines)\n",
		       st.orig.filename, st.orig.nlines);
	} else {
		/* Multi-file mode */
		if (buffer_load_multi(&st.orig, files, nfiles) < 0) {
			fprintf(stderr, "xf: cannot load files\n");
			return 1;
		}
		printf("xf: loaded %zu files (%zu lines total)\n",
			nfiles, st.orig.nlines);
		for (i = 0; i < st.orig.nregions; i++) {
			size_t len = st.orig.regions[i].end_line -
				st.orig.regions[i].start_line + 1;
			printf("  %s (%zu lines, lines %zu-%zu)\n",
				st.orig.regions[i].filename, len,
				st.orig.regions[i].start_line + 1,
				st.orig.regions[i].end_line + 1);
		}
	}

	selection_init(&st.sel);
	selection_table_init(&st.seltbl);
	history_init(&st.hist);

	if (buffer_dup(&st.staged, &st.orig) < 0) {
		fprintf(stderr, "xf: out of memory\n");
		buffer_free(&st.orig);
		return 1;
	}

	if (replay_path != NULL)
		replay_file(&st, replay_path);

	if (batch_path != NULL) {
		replay_file(&st, batch_path);
		st.running = 0;
	}

	/* Interactive REPL (only if not batch mode) */
	while (st.running) {
		printf("> ");
		fflush(stdout);

		if (getline(&line, &linecap, stdin) == -1)
			break;

		line[strcspn(line, "\n")] = '\0';
		execute_command(&st, line, 1);
	}

	free(line);
	selection_free(&st.sel);
	selection_table_free(&st.seltbl);
	history_free(&st.hist);
	buffer_free(&st.orig);
	buffer_free(&st.staged);

	return 0;
}

int
main(int argc, char *argv[])
{
	const char **files = NULL;
	size_t nfiles = 0;
	size_t files_cap = 0;
	const char *replay_path = NULL;
	const char *batch_path = NULL;
	int i;
	int rc = 0;

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--replay") == 0) {
			if (i + 1 >= argc) {
				fprintf(stderr, "xf: --replay requires a file\n");
				return 1;
			}
			replay_path = argv[++i];
		} else if (strcmp(argv[i], "--batch") == 0) {
			if (i + 1 >= argc) {
				fprintf(stderr, "xf: --batch requires a file\n");
				return 1;
			}
			batch_path = argv[++i];
		} else if (argv[i][0] == '-') {
			fprintf(stderr, "xf: unknown option '%s'\n", argv[i]);
			print_usage();
			return 1;
		} else {
			/* Collect filename */
			if (nfiles >= files_cap) {
				size_t newcap = files_cap == 0 ? 8 : files_cap * 2;
				const char **newfiles = realloc(files,
					newcap * sizeof(const char *));
				if (newfiles == NULL) {
					fprintf(stderr, "xf: out of memory\n");
					free(files);
					return 1;
				}
				files = newfiles;
				files_cap = newcap;
			}
			files[nfiles++] = argv[i];
		}
	}

	if (nfiles == 0) {
		print_usage();
		free(files);
		return 1;
	}

	/* Process all files together */
	rc = process_files(files, nfiles, replay_path, batch_path);

	free(files);
	return rc;
}
