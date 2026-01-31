# xf -- User Guide

A comprehensive guide to using **xf**, the intent-driven terminal text editor.

---

## Table of Contents

1. [Core Concepts](#core-concepts)
2. [Complete Examples](#complete-examples)
3. [Batch Mode and Scripting](#batch-mode-and-scripting)
4. [Multi-File Editing](#multi-file-editing)
5. [Tips and Best Practices](#tips-and-best-practices)
6. [Troubleshooting](#troubleshooting)

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
