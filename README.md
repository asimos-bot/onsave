## Onsave

Run command when there is a change to files or directories.

## Build

Requirements:
* `gcc` or `clang` (change in the `Makefile` please)
* `libnotify-dev`

```
make build
```

## Usage

Usage: `onsave [OPTIONS] [FILES ...]`

* `-o / --once` - run only once
* `-h / --help` - help menu
* `--exclude` - don't watch these files / directories
* `-g / --git` - git only
* `-f / --flags` - choose flags to watch 
