# XConfig

## Overview

XConfig is a simple library for parsing config files like this:
```
[Section]
Key = "Value"
```

## Build

### Required libraries & tools
1. **gcc**: GCC Version 12.0.0+ is recommended
2. **make**: Used to build projects
3. **clang**: Optional, if you couldn't install GCC

### Installation steps

**Debian/Ubuntu**
```bash
sudo apt update && sudo apt install make gcc # clang
```

**ArchLinux/Manjaro**
```bash
sudo pacman -Sy make gcc # clang
```

**Termux**
```bash
pkg update && pkg install make clang
```

## Build steps

### Clone this project

```bash
git clone https://github.com/ASO-Studio/XConfig
```

### Compile
```bash
make
```

## Usage

### See [docs](./docs/docs.md)
