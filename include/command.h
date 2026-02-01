#ifndef COMMAND_H
#define COMMAND_H

/* Command types */
enum cmd_type {
	CMD_NONE,
	CMD_SELECT,
	CMD_REFINE,
	CMD_REPLACE,
	CMD_DELETE,
	CMD_PREVIEW,
	CMD_COMMIT,
	CMD_ABORT,
	CMD_QUIT,
	CMD_HELP,
	CMD_SELECTIONS,
	CMD_HISTORY,
	CMD_SAVE,
	CMD_REPLAY,
	CMD_INSERT,
	CMD_MOVE,
	CMD_UNKNOWN
};

/* Pattern type */
enum pattern_type {
	PAT_NONE,
	PAT_STRING,
	PAT_REGEX
};

/* Parsed command */
struct command {
	enum cmd_type type;
	enum pattern_type pat_type;
	char *arg1;
	char *arg2;
	char *arg3;
	char *sel_name;  /* for "as <name>" */
};

/* Parse a command line, returns command struct */
struct command command_parse(const char *line);

/* Free command resources */
void command_free(struct command *cmd);

/* Get command name as string */
const char *command_name(enum cmd_type type);

#endif /* COMMAND_H */
