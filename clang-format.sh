#!/bin/bash

function format_files()
{
    for f in $1
    do
        if [ -f "$f" ]; then
            clang-format -i "$f"
        fi
    done
}

format_files "main.c"
format_files "ops.c"
format_files "db.c"
format_files "tlm.c"


format_files "ops.h"
format_files "db.h"
format_files "tlm.h"
