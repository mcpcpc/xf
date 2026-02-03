# xf -- User Guide

A comprehensive guide to using **xf**, the intent-driven terminal text editor.

---

## Table of Contents

1. [Core Concepts](#core-concepts)
2. [Complete Examples](#complete-examples)
3. [Batch Mode and Scripting](#batch-mode-and-scripting)
4. [Multi-File Editing](#multi-file-editing)
5. [Tips and Best Practices](#tips-and-best-practices)
6. [Why Staging Matters](#why-staging-matters)
7. [The Real Power of xf](#the-real-power-of-xf)
8. [Troubleshooting](#troubleshooting)

---

## Core Concepts

### Selections

A **selection** is a set of line ranges that your commands will operate on. Selections can span:
- Individual lines
- Paragraphs (blocks of text separated by blank lines)
- Indentation-based blocks
- C-style functions

Selections are:
- **Named**: You can save selections with names for later use
- **Composable**: You can refine selections to narrow them down
- **Non-destructive**: Creating a selection doesn't modify anything

### Transactional Editing

xf never modifies your file directly. Instead:

1. You create a **selection**
2. You apply a **transformation** (the change is "staged")
3. You **preview** the staged changes as a diff
4. You **commit** to write changes to disk, or **abort** to discard them

This ensures you always know exactly what will change before it happens.

### Patterns

xf supports two types of patterns:

| Syntax | Type | Example |
|--------|------|---------|
| `"pattern"` | Substring match | `"malloc"` matches any line containing "malloc" |
| `/regex/` | Regular expression | `/^#include/` matches lines starting with "#include" |

Both single and double quotes work for string patterns.

For the complete command reference, see `man xf`.

---

## Complete Examples

### Example 1: Renaming a Function

Rename all occurrences of `old_function` to `new_function`:

```
$ xf mycode.c
xf: loaded mycode.c (200 lines)

> select lines matching "old_function"
xf: 15 range(s), 15 line(s) selected
     1: | void old_function(int x)
     2: |     old_function(0);
     3: |     result = old_function(value);
  ... (12 more lines)

> replace "old_function" with "new_function"
xf: 15 replacement(s) staged

> preview
--- original
+++ modified
@@ -10,1 +10,1 @@
-void old_function(int x)
+void new_function(int x)
@@ -25,1 +25,1 @@
-    old_function(0);
+    new_function(0);
...

> commit
xf: committed 200 lines to mycode.c

> quit
```

### Example 2: Removing Debug Statements

Remove all lines containing debug prints:

```
$ xf app.c
xf: loaded app.c (500 lines)

> select lines matching "printf"
xf: 45 range(s), 45 line(s) selected

> refine containing "DEBUG"
xf: 12 range(s), 12 line(s) selected
     1: |     printf("DEBUG: x = %d\n", x);
     2: |     printf("DEBUG: entering loop\n");
  ... (10 more lines)

> delete
xf: 12 line(s) deleted

> preview
--- original
+++ modified
@@ -15,1 +15,0 @@
-    printf("DEBUG: x = %d\n", x);
@@ -28,1 +27,0 @@
-    printf("DEBUG: entering loop\n");
...

> commit
xf: committed 488 lines to app.c

> quit
```

### Example 3: Adding Copyright Headers to Functions

Add a comment before each function:

```
$ xf source.c
xf: loaded source.c (300 lines)

> select functions
xf: 20 range(s), 280 line(s) selected

> insert "/* Copyright 2024 Example Corp. */" before
xf: 20 line(s) inserted

> preview
--- original
+++ modified
@@ -10,0 +10,1 @@
+/* Copyright 2024 Example Corp. */
 int calculate_sum(int a, int b)
@@ -25,0 +26,1 @@
+/* Copyright 2024 Example Corp. */
 void process_data(void *data)
...

> commit
xf: committed 320 lines to source.c

> quit
```

### Example 4: Complex Refactoring with Named Selections

Replace `malloc` with `safe_malloc` only in non-static functions:

```
$ xf memory.c
xf: loaded memory.c (400 lines)

> select functions as all_funcs
xf: [all_funcs] 25 range(s), 380 line(s) selected

> refine excluding /^static/
xf: 12 range(s), 180 line(s) selected

> refine containing "malloc" as public_malloc_funcs
xf: [public_malloc_funcs] 5 range(s), 60 line(s) selected

> replace "malloc" with "safe_malloc"
xf: 8 replacement(s) staged

> preview
--- original
+++ modified
@@ -50,1 +50,1 @@
-    buf = malloc(size);
+    buf = safe_malloc(size);
...

> commit
xf: committed 400 lines to memory.c

> quit
```

### Example 5: Extracting TODO Items

Find all TODO comments and save them:

```
$ xf project.c
xf: loaded project.c (1000 lines)

> select lines matching "TODO"
xf: 15 range(s), 15 line(s) selected
     1: | // TODO: implement error handling
     2: | // TODO: add unit tests
     3: | /* TODO: optimize this algorithm */
     4: | // TODO: document this function
     5: | // TODO: refactor into smaller functions
  ... (10 more lines)

> history
xf: 1 command(s) in history
    1: select lines matching "TODO"

> save todo_search.xf
xf: saved 1 command(s) to todo_search.xf

> quit
```

---

## Batch Mode and Scripting

### The `--batch` Option

Run xf non-interactively with a script file:

```bash
$ cat refactor.xf
# Refactor script: replace malloc with xmalloc
select functions containing "malloc"
replace "malloc" with "xmalloc"
commit

$ xf --batch refactor.xf mycode.c
xf: loaded mycode.c (200 lines)
xf: replaying 3 command(s) from refactor.xf
> select functions containing "malloc"
xf: 5 range(s), 60 line(s) selected
> replace "malloc" with "xmalloc"
xf: 8 replacement(s) staged
> commit
xf: committed 200 lines to mycode.c
```

### The `--replay` Option

Replay a script and then enter interactive mode:

```bash
$ xf --replay setup.xf mycode.c
xf: loaded mycode.c (200 lines)
xf: replaying 2 command(s) from setup.xf
> select functions
xf: 15 range(s), 180 line(s) selected
> refine containing "error"
xf: 3 range(s), 36 line(s) selected

# Now in interactive mode
> replace "error" with "err"
xf: 5 replacement(s) staged
> ...
```

### Script File Format

Script files are plain text with one command per line. Comments start with `#`:

```
# This is a refactoring script
# Created for the XYZ project

# Step 1: Select all malloc calls
select functions containing "malloc"

# Step 2: Replace malloc with xmalloc
replace "malloc" with "xmalloc"

# Step 3: Commit changes
commit

# Step 4: Remove debug lines
select lines matching "DEBUG"
delete
commit
```

### Using xf in Shell Pipelines

Create reusable refactoring scripts:

```bash
#!/bin/bash
# refactor-malloc.sh - Replace malloc with xmalloc in C files

for file in "$@"; do
    echo "Processing $file..."
    xf --batch refactor.xf "$file"
done
```

---

## Multi-File Editing

xf can edit multiple files simultaneously. All files are loaded into a unified buffer, and commands operate across all files.

### Loading Multiple Files

```bash
$ xf src/*.c
xf: loaded 5 files (850 lines total)
  src/main.c (200 lines, lines 1-200)
  src/utils.c (150 lines, lines 201-350)
  src/parser.c (300 lines, lines 351-650)
  src/lexer.c (100 lines, lines 651-750)
  src/output.c (100 lines, lines 751-850)
```

### Editing Across Files

Commands work across all loaded files:

```
> select lines matching "malloc"
xf: 25 range(s), 25 line(s) selected
     1: |     buf = malloc(size);
     2: |     str = malloc(len + 1);
     3: |     node = malloc(sizeof(*node));
  ... (22 more lines)

> replace "malloc" with "xmalloc"
xf: 25 replacement(s) staged

> commit
xf: committed 200 lines to src/main.c
xf: committed 150 lines to src/utils.c
xf: committed 300 lines to src/parser.c
xf: committed 100 lines to src/lexer.c
xf: committed 100 lines to src/output.c
```

---

## Tips and Best Practices

### 1. Always Preview Before Committing

Get in the habit of running `preview` before `commit`:

```
> replace "foo" with "bar"
xf: 10 replacement(s) staged

> preview      # ← Always do this first
--- original
+++ modified
...

> commit
```

### 2. Use Named Selections for Complex Workflows

When performing multi-step operations, name your selections:

```
> select functions as all_funcs
> refine containing "malloc" as malloc_funcs
> refine excluding comments as code_only
> replace "malloc" with "xmalloc"
```

### 3. Build Up Selections Incrementally

Start broad and refine:

```
> select paragraphs              # Start with all paragraphs
> refine containing "TODO"       # Narrow to those with TODO
> refine excluding comments      # Exclude comment-only blocks
> delete                         # Delete what remains
```

### 4. Save Your Workflows

When you find a useful command sequence, save it:

```
> history
> save my_refactor.xf
```

Then reuse it on other files:

```bash
$ xf --batch my_refactor.xf other_file.c
```

### 5. Use Regex for Complex Patterns

Substring matching is fast but limited. Use regex for complex cases:

```
# Match function declarations (simplified)
> select lines matching /^[a-z_]+[[:space:]]+[a-z_]+\(/

# Match lines starting with whitespace followed by "if"
> select lines matching /^[[:space:]]+if[[:space:]]*\(/

# Match either TODO or FIXME
> select lines matching /TODO|FIXME/
```

### 6. Abort When Unsure

If a preview shows unexpected changes, abort:

```
> replace "ptr" with "pointer"
xf: 50 replacement(s) staged

> preview
# Hmm, this is changing too much...

> abort
xf: changes discarded
```

---

## Why Staging Matters

The staging model isn't just a safety feature—it fundamentally changes how you approach text editing. Here's why it matters:

### Catching Unintended Matches

**The Problem**: Pattern matching often catches more than you expect.

```
$ xf database.c
xf: loaded database.c (800 lines)

> select lines matching "id"
xf: 47 range(s), 47 line(s) selected
     1: |     int user_id = get_user_id();
     2: |     validate_credentials(user_id);
     3: |     // Consider adding UUID support
     4: |     void *ptr = malloc(sizeof(id_t));
     5: |     printf("Invalid input provided\n");
  ... (42 more lines)

> replace "id" with "identifier"
xf: 52 replacement(s) staged

> preview
--- original
+++ modified
@@ -15,1 +15,1 @@
-    int user_id = get_user_id();
+    int user_identifier = get_user_identifier();
@@ -42,1 +42,1 @@
-    // Consider adding UUID support
+    // Considentifierer adding UUidentifier support
@@ -89,1 +89,1 @@
-    printf("Invalid input provided\n");
+    printf("Invalidentifier input providentifiered\n");
...

# Disaster avoided! "id" appears inside many words
> abort
xf: changes discarded

# More precise approach:
> select lines matching "user_id"
xf: 12 range(s), 12 line(s) selected

> replace "user_id" with "user_identifier"
xf: 15 replacement(s) staged

> preview
--- original
+++ modified
@@ -15,1 +15,1 @@
-    int user_id = get_user_id();
+    int user_identifier = get_user_identifier();
...

> commit
xf: committed 800 lines to database.c
```

Without staging, the first replace would have corrupted your file.

### Scope Creep in Refactoring

**The Problem**: A "simple" rename touches more than expected.

```
$ xf config.c
xf: loaded config.c (400 lines)

> select functions
xf: 18 range(s), 380 line(s) selected

> refine containing "config"
xf: 8 range(s), 120 line(s) selected

> replace "config" with "settings"
xf: 24 replacement(s) staged

> preview
--- original
+++ modified
@@ -10,1 +10,1 @@
-void load_config(const char *path)
+void load_settings(const char *path)
@@ -15,1 +15,1 @@
-    config_t *cfg = parse_config(path);
+    settings_t *cfg = parse_settings(path);
@@ -45,1 +45,1 @@
-    // TODO: reconfigure logging
+    // TODO: resettingsure logging
...

# Wait—"reconfigure" contains "config" too!
> abort
xf: changes discarded

# Use word boundaries in regex:
> select functions containing "config"
xf: 8 range(s), 120 line(s) selected

> replace /\bconfig\b/ with "settings"
xf: 18 replacement(s) staged

> preview
--- original
+++ modified
@@ -10,1 +10,1 @@
-void load_config(const char *path)
+void load_settings(const char *path)
@@ -45,1 +45,1 @@
     // TODO: reconfigure logging
...

# "reconfigure" is untouched now
> commit
xf: committed 400 lines to config.c
```

### Multi-File Safety Net

**The Problem**: Changes across multiple files amplify risk.

```
$ xf src/*.c
xf: loaded 12 files (4500 lines total)

> select lines matching "ERROR_CODE"
xf: 89 range(s), 89 line(s) selected

> replace "ERROR_CODE" with "ERR_CODE"
xf: 89 replacement(s) staged

> preview
--- original
+++ modified
@@ -25,1 +25,1 @@ src/main.c
-    return ERROR_CODE_INVALID;
+    return ERR_CODE_INVALID;
@@ -102,1 +102,1 @@ src/parser.c
-#define ERROR_CODE_BASE 1000
+#define ERR_CODE_BASE 1000
...

# 89 changes across 12 files—review carefully before committing
> commit
xf: committed 350 lines to src/main.c
xf: committed 400 lines to src/parser.c
...
```

Imagine if those 89 changes went straight to disk without review.

---

## The Real Power of xf

The examples so far show simple search-and-replace. But xf's true strength lies in **structural awareness** combined with **selection composition**—capabilities that make complex transformations safe and precise.

### Surgical Code Modification

**Goal**: Update only public API functions, leaving internal helpers alone.

```
$ xf api.c
xf: loaded api.c (600 lines)

> select functions as all_funcs
xf: [all_funcs] 25 range(s), 580 line(s) selected

# Exclude static (internal) functions
> refine excluding /^static/
xf: 12 range(s), 290 line(s) selected

# Keep only functions that return error codes
> refine containing "return -1" as public_error_funcs
xf: [public_error_funcs] 5 range(s), 85 line(s) selected
     1: | int api_connect(const char *host) {
        |     ...
        |     return -1;
        | }
     2: | int api_send(int fd, const void *data) {
        |     ...
        |     return -1;
        | }
  ... (3 more functions)

# Now add proper error logging to just these functions
> replace "return -1" with "log_error(__func__); return -1"
xf: 8 replacement(s) staged

> preview
--- original
+++ modified
@@ -45,1 +45,1 @@
-        return -1;
+        log_error(__func__); return -1;
...

> commit
xf: committed 600 lines to api.c
```

This targets exactly the right code: public functions that return errors.

### Codebase-Wide API Migration

**Goal**: Replace deprecated API across an entire project, but only in actual code (not comments or strings).

```
$ xf src/*.c include/*.h
xf: loaded 45 files (12000 lines total)

> select lines matching "pthread_create"
xf: 34 range(s), 34 line(s) selected
     1: |     pthread_create(&thread, NULL, worker, arg);
     2: |     // Old: pthread_create is deprecated
     3: |     ret = pthread_create(&t, &attr, handler, ctx);
     4: |     printf("Using pthread_create\n");
  ... (30 more lines)

# Remove comments—we don't want to change documentation
> refine excluding comments
xf: 28 range(s), 28 line(s) selected

# Remove string literals—don't change log messages
> refine excluding /".*pthread_create.*"/
xf: 26 range(s), 26 line(s) selected

# Now we have only actual pthread_create calls
> replace "pthread_create" with "thread_pool_spawn"
xf: 26 replacement(s) staged

> preview
--- original
+++ modified
@@ -89,1 +89,1 @@ src/worker.c
-    pthread_create(&thread, NULL, worker, arg);
+    thread_pool_spawn(&thread, NULL, worker, arg);
@@ -45,1 +45,1 @@ src/handler.c
     // Old: pthread_create is deprecated   ← unchanged
-    ret = pthread_create(&t, &attr, handler, ctx);
+    ret = thread_pool_spawn(&t, &attr, handler, ctx);
@@ -102,1 +102,1 @@ src/main.c
     printf("Using pthread_create\n");      ← unchanged
...

> commit
xf: committed 45 files
```

### Function-Level Transformations

**Goal**: Add error checking to all malloc calls inside a specific function.

```
$ xf memory.c
xf: loaded memory.c (500 lines)

> select functions containing "parse_input" as target_func
xf: [target_func] 1 range(s), 45 line(s) selected
     1: | int parse_input(const char *input) {
        |     char *buf = malloc(strlen(input) + 1);
        |     token_t *tok = malloc(sizeof(token_t));
        |     ...
        | }

# Now select just the malloc lines within this function
> refine containing "malloc"
xf: 3 range(s), 3 line(s) selected
     1: |     char *buf = malloc(strlen(input) + 1);
     2: |     token_t *tok = malloc(sizeof(token_t));
     3: |     node_t *n = malloc(sizeof(node_t));

> insert "if (!ptr) return -1;  /* allocation check */" after
xf: 3 line(s) inserted

> preview
--- original
+++ modified
@@ -112,1 +112,2 @@
     char *buf = malloc(strlen(input) + 1);
+    if (!ptr) return -1;  /* allocation check */
@@ -115,1 +116,2 @@
     token_t *tok = malloc(sizeof(token_t));
+    if (!ptr) return -1;  /* allocation check */
...

> commit
xf: committed 503 lines to memory.c
```

### Paragraph-Aware Documentation Edits

**Goal**: Find and update specific documentation sections.

```
$ xf docs/api.md
xf: loaded docs/api.md (800 lines)

> select paragraphs containing "deprecated"
xf: 4 range(s), 28 line(s) selected
     1: | The `old_api()` function is deprecated.
        | Use `new_api()` instead. This function
        | will be removed in version 3.0.
     2: | Note: The deprecated `legacy_mode` flag
        | should not be used in new code.
  ... (2 more paragraphs)

> insert "**WARNING: DEPRECATED**" before
xf: 4 line(s) inserted

> preview
--- original
+++ modified
@@ -45,0 +45,1 @@
+**WARNING: DEPRECATED**
 The `old_api()` function is deprecated.
 Use `new_api()` instead. This function
 will be removed in version 3.0.
...

> commit
xf: committed 804 lines to docs/api.md
```

### Indentation-Aware Block Operations

**Goal**: Extract deeply nested code blocks for refactoring analysis.

```
$ xf complex.c
xf: loaded complex.c (1200 lines)

> select blocks
xf: 156 range(s), 1180 line(s) selected

# Find blocks with excessive nesting (4+ levels = 32+ spaces with 8-space tabs)
> refine containing /^[[:space:]]{32,}[^[:space:]]/
xf: 8 range(s), 95 line(s) selected
     1: |                                 if (deeply_nested) {
        |                                     handle_edge_case();
        |                                 }
  ... (7 more deeply nested blocks)

# These are refactoring candidates—add TODO markers
> insert "/* TODO: refactor - excessive nesting */" before
xf: 8 line(s) inserted

> commit
xf: committed 1208 lines to complex.c
```

### Composing Selections for Complex Queries

**Goal**: Find functions that use malloc but don't check the return value.

```
$ xf src/*.c
xf: loaded 20 files (8000 lines total)

> select functions containing "malloc" as malloc_funcs
xf: [malloc_funcs] 45 range(s), 890 line(s) selected

# Keep functions that have malloc but DON'T have null checks
> refine excluding "if (!ptr)" as unchecked
xf: [unchecked] 12 range(s), 180 line(s) selected

> refine excluding "if (ptr == NULL)" as truly_unchecked
xf: [truly_unchecked] 8 range(s), 120 line(s) selected
     1: | void leak_example() {
        |     char *p = malloc(100);
        |     strcpy(p, "data");  // no null check!
        |     ...
        | }
  ... (7 more unsafe functions)

# Flag these for review
> insert "/* FIXME: malloc return value not checked */" before
xf: 8 line(s) inserted

> commit
xf: committed files
```

### Reproducible Transformations

**Goal**: Apply the same complex refactoring to multiple projects.

```bash
# Create a reusable refactoring script
$ cat > modernize_error_handling.xf << 'EOF'
# Modernize error handling pattern
# Replaces: if (err) { return err; }
# With: if (err) return err;

select functions
refine containing "if (err)"
refine containing "return err"
replace /if \(err\) \{\n[[:space:]]*return err;\n[[:space:]]*\}/ with "if (err) return err;"
commit
EOF

# Apply to all C files in project A
$ xf --batch modernize_error_handling.xf ~/project_a/src/*.c

# Apply to project B
$ xf --batch modernize_error_handling.xf ~/project_b/src/*.c

# Apply to project C
$ xf --batch modernize_error_handling.xf ~/project_c/src/*.c
```

The same transformation, applied consistently across multiple codebases.

---

## Troubleshooting

### "xf: no selection"

You tried to run a transformation without first selecting text:

```
> replace "foo" with "bar"
xf: no selection
```

**Solution**: Run a `select` command first:

```
> select lines matching "foo"
> replace "foo" with "bar"
```

### "xf: no staged changes"

You tried to preview or commit without making changes:

```
> preview
xf: no staged changes
```

**Solution**: Run a transformation command (`replace`, `delete`, `insert`, `move`) first.

### "xf: invalid regex pattern"

Your regular expression has a syntax error:

```
> select lines matching /[unclosed/
xf: invalid regex pattern
```

**Solution**: Fix your regex. xf uses POSIX Extended Regular Expressions.

### Selection is empty after refine

Your refinement excluded everything:

```
> select lines matching "malloc"
xf: 10 range(s), 10 line(s) selected

> refine containing "nonexistent_pattern"
xf: 0 range(s), 0 line(s) selected
```

**Solution**: Start over with `select` or use a different pattern.

### Changes not appearing in file

You forgot to commit:

```
> replace "foo" with "bar"
xf: 5 replacement(s) staged

> quit
# Oops! Changes were never committed
```

**Solution**: Always run `commit` before `quit` to save changes:

```
> replace "foo" with "bar"
> commit
> quit
```

---

For a complete command reference, see `man xf` after installation.

---

*xf -- intent-driven editing for the terminal*
