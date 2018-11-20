#!/bin/sh
scripts/checkpatch.pl --no-tree --ignore CONST_STRUCT,SPDX_LICENSE_TAG,LINE_CONTINUATIONS -f $1
