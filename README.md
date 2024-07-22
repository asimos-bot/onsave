## Onsave

Lightweight and straightforward program that executes a given command when there is a change to a file or directory.

Just tell it which file or directory to monitor and what command to execute afterwards:

```
onsave README.md echo hello
```

### Build

Requirements:
* `gcc` or `clang` (you may change it in the `Makefile`)
* `libnotify-dev` package (in debian/ubuntu) or equivalente for your system

```
make build
```

## Todo

- [x] watch events
- [x] `-o` flag to only run command once
- [x] consume watch event only for
    * IN_MOVE_SELF
    * IN_MOVE
    * IN_ATTRIB
    * IN_MODIFY
    * IN_DELETE_SELF
- [x] `-v` flag to show extra info
- [x] `-q` flag to show extra info
- [x] reapply watcher for events like:
    * IN_DELETE_SELF
- [x] ignore all files given with the `-i` flag
- [ ] recursive directory monitoring

## Troubleshooting

Vim and neovim users may want to set `backupcopy` to `false` to avoid false positives due to the "4913" file.
