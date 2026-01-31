# xf

**xf** is an intent-driven terminal text editor.

What, not where. No cursor. No modes. No implicit mutation.

## Requirements

- C99 compiler (gcc, clang)
- POSIX-compatible system (Linux, macOS, BSD)
- make

## Installation

### Build from source

```bash
git clone https://github.com/mcpcpc/xf.git
cd xf
make
```

The `xf` executable will be created in the current directory.

### Install to system

```bash
sudo make install
```

This installs xf to `/usr/local/bin` by default. To customize the installation path:

```bash
make PREFIX=/opt/xf install
```

### Uninstall

```bash
sudo make uninstall
```

## Quick Example

```
$ xf myfile.c
xf: loaded myfile.c (156 lines)

> select lines matching "TODO"
xf: 5 range(s), 5 line(s) selected

> replace "TODO" with "DONE"
xf: 5 replacement(s) staged

> preview
--- original
+++ modified
@@ -10,1 +10,1 @@
-// TODO: implement error handling
+// DONE: implement error handling
...

> commit
xf: committed 156 lines to myfile.c

> quit
```

## Documentation

- **[USAGE.md](USAGE.md)** -- Tutorial and user guide
- **man xf** -- Command reference (after installation)

## License

BSD 3-Clause License. See [LICENSE](LICENSE) for details.

## Author

Michael Czigler
