#!/bin/bash
set -e

function atexit {
    cd "$old_dir"
}

help_msg="Usage: $(basename "$0") [standard]\nAvailable standards:\n  c++17\n  c++20\n  c++23"

old_dir=$(pwd)
cd "$(dirname "$0")"
trap atexit EXIT

if [ "$1" = "--help" ]; then
    echo -e "$help_msg"
    exit 0
elif [ "$1" = "c++17" ]; then
    doxyfile=cpp17
elif [ "$1" = "c++20" ]; then
    doxyfile=cpp20
elif [ "$1" = "c++23" ]; then
    doxyfile=cpp23
else
    echo >&2 "Invalid arguments. Try $(basename "$0") --help"
    exit 1
fi

doxygen "$doxyfile"
