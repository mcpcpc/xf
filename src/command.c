#include "command.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

static char *
skip_whitespace(char *s)
{
	while (*s && isspace((unsigned char)*s))
		s++;
	return s;
}

static char *
extract_word(char **s)
{
	char *start, *end, *word;
	size_t len;

	*s = skip_whitespace(*s);
	if (**s == '\0')
		return NULL;

	start = *s;
	while (**s && !isspace((unsigned char)**s))
		(*s)++;
	end = *s;

	len = end - start;
	word = malloc(len + 1);
	if (word == NULL)
		return NULL;
	memcpy(word, start, len);
	word[len] = '\0';

	return word;
}

/* Extract quoted string or regex /pattern/ */
static char *
extract_pattern(char **s, enum pattern_type *ptype)
{
	char *start, *end, *result;
	size_t len;
	char delim;

	*s = skip_whitespace(*s);
	*ptype = PAT_NONE;

	if (**s == '/') {
		/* Regex pattern */
		delim = '/';
		*ptype = PAT_REGEX;
	} else if (**s == '"' || **s == '\'') {
		/* String pattern */
		delim = **s;
		*ptype = PAT_STRING;
	} else {
		/* Plain word */
		*ptype = PAT_STRING;
		return extract_word(s);
	}

	(*s)++;
	start = *s;

	while (**s && **s != delim)
		(*s)++;

	end = *s;
	if (**s == delim)
		(*s)++;

	len = end - start;
	result = malloc(len + 1);
	if (result == NULL)
		return NULL;
	memcpy(result, start, len);
	result[len] = '\0';

	return result;
}

static char *
extract_quoted(char **s, enum pattern_type *ptype)
{
	return extract_pattern(s, ptype);
}

/* Check for and extract "as <name>" at end of command */
static char *
extract_as_name(char **s)
{
	char *p, *name;

	p = skip_whitespace(*s);
	if (strncmp(p, "as", 2) == 0 && isspace((unsigned char)p[2])) {
		p += 2;
		p = skip_whitespace(p);
		name = extract_word(&p);
		*s = p;
		return name;
	}
	return NULL;
}

struct command
command_parse(const char *line)
{
	struct command cmd;
	char *buf, *p;
	char *verb;

	memset(&cmd, 0, sizeof(cmd));
	cmd.type = CMD_NONE;
	cmd.pat_type = PAT_NONE;

	buf = strdup(line);
	if (buf == NULL)
		return cmd;

	p = buf;
	verb = extract_word(&p);
	if (verb == NULL) {
		free(buf);
		return cmd;
	}

	if (strcmp(verb, "select") == 0) {
		cmd.type = CMD_SELECT;
		cmd.arg1 = extract_word(&p);    /* type: lines, paragraphs */
		cmd.arg2 = extract_word(&p);    /* modifier: matching, containing */
		cmd.arg3 = extract_quoted(&p, &cmd.pat_type);  /* pattern */
		cmd.sel_name = extract_as_name(&p);
	} else if (strcmp(verb, "refine") == 0) {
		cmd.type = CMD_REFINE;
		cmd.arg1 = extract_word(&p);    /* containing/excluding */
		cmd.arg2 = extract_quoted(&p, &cmd.pat_type);  /* pattern */
	} else if (strcmp(verb, "replace") == 0) {
		enum pattern_type dummy;
		cmd.type = CMD_REPLACE;
		cmd.arg1 = extract_quoted(&p, &dummy);  /* old string */
		p = skip_whitespace(p);
		/* Skip "with" if present */
		if (strncmp(p, "with", 4) == 0 && isspace((unsigned char)p[4])) {
			p += 4;
		}
		cmd.arg2 = extract_quoted(&p, &dummy);  /* new string */
	} else if (strcmp(verb, "delete") == 0) {
		cmd.type = CMD_DELETE;
	} else if (strcmp(verb, "preview") == 0) {
		cmd.type = CMD_PREVIEW;
	} else if (strcmp(verb, "commit") == 0) {
		cmd.type = CMD_COMMIT;
	} else if (strcmp(verb, "abort") == 0) {
		cmd.type = CMD_ABORT;
	} else if (strcmp(verb, "quit") == 0 || strcmp(verb, "q") == 0) {
		cmd.type = CMD_QUIT;
	} else if (strcmp(verb, "help") == 0 || strcmp(verb, "?") == 0) {
		cmd.type = CMD_HELP;
	} else if (strcmp(verb, "selections") == 0) {
		cmd.type = CMD_SELECTIONS;
	} else if (strcmp(verb, "history") == 0) {
		cmd.type = CMD_HISTORY;
	} else if (strcmp(verb, "save") == 0) {
		cmd.type = CMD_SAVE;
		cmd.arg1 = extract_word(&p);  /* filename */
	} else if (strcmp(verb, "replay") == 0) {
		cmd.type = CMD_REPLAY;
		cmd.arg1 = extract_word(&p);  /* filename */
	} else if (strcmp(verb, "insert") == 0) {
		enum pattern_type dummy;
		cmd.type = CMD_INSERT;
		cmd.arg1 = extract_quoted(&p, &dummy);  /* text to insert */
		cmd.arg2 = extract_word(&p);  /* before/after */
	} else if (strcmp(verb, "move") == 0) {
		cmd.type = CMD_MOVE;
		cmd.arg1 = extract_word(&p);  /* before/after/to */
		cmd.arg2 = extract_word(&p);  /* line number */
	} else {
		cmd.type = CMD_UNKNOWN;
	}

	free(verb);
	free(buf);

	return cmd;
}

void
command_free(struct command *cmd)
{
	free(cmd->arg1);
	free(cmd->arg2);
	free(cmd->arg3);
	free(cmd->sel_name);
	memset(cmd, 0, sizeof(*cmd));
}

const char *
command_name(enum cmd_type type)
{
	switch (type) {
	case CMD_SELECT:     return "select";
	case CMD_REFINE:     return "refine";
	case CMD_REPLACE:    return "replace";
	case CMD_DELETE:     return "delete";
	case CMD_PREVIEW:    return "preview";
	case CMD_COMMIT:     return "commit";
	case CMD_ABORT:      return "abort";
	case CMD_QUIT:       return "quit";
	case CMD_HELP:       return "help";
	case CMD_SELECTIONS: return "selections";
	case CMD_HISTORY:    return "history";
	case CMD_SAVE:       return "save";
	case CMD_REPLAY:     return "replay";
	case CMD_INSERT:     return "insert";
	case CMD_MOVE:       return "move";
	default:             return "unknown";
	}
}
