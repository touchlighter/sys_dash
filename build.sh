#!/bin/zsh
set -e
g++ -std=c++17 -O2 main.cpp sysinfo.cpp -o sysdash \
    -lncurses -framework IOKit -framework CoreFoundation
echo "[✓] Build successful. Run with ./sysdash"

