#!/bin/bash
#
# Copyright (c) 2017 The tyndur Project. All rights reserved.
#
# This code is derived from software contributed to the tyndur Project
# by Kevin Wolf.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
# OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
# WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

if [ $# != 2 ]; then
    echo "Syntax: $0 <package.tar> <root-dir>"
    exit 1
fi

path="$1"
filename=$(basename "$path")
if [[ "$filename" =~ ^(.*)-lib-([^-]*)-i386\.tar$ ]]; then
    is_lib=1
else
    is_lib=0
    [[ "$filename" =~ ^(.*)-([^-]*)\.tar$ ]]

fi

echo "Paket: ${BASH_REMATCH[1]}, Version: ${BASH_REMATCH[2]}"

for j in $(seq 1 5); do
    unset "info[$j]"
done
readarray -tO1 info < <(tar --wildcards -xOf $path packages/${BASH_REMATCH[1]}/${BASH_REMATCH[2]}/packageinfo-*)
name=${info[1]:-"${BASH_REMATCH[1]}"}
version=${info[2]:-"${BASH_REMATCH[2]}"}
section=${info[3]:-bin}
arch=${info[4]:-i386}
desc=${info[5]:-"Keine Beschreibung"}

tmpfile=$(mktemp --tmpdir lpt_install.XXXXXX)
trap 'rm "$tmpfile"' INT TERM HUP EXIT

tar --wildcards -xOf "$path" "packages/${BASH_REMATCH[1]}/${BASH_REMATCH[2]}/postinstall-*" | grep -v '^#!file:/apps/sh$' > "$tmpfile"

if grep -v "^lpt cfg-add\(bin\|doc\|inc\|lib\)" $tmpfile; then
    echo "Kann Postinstall-Skript nicht anwenden"
    exit 1
fi

while read -r line; do
    if ! [[ "$line" =~ ^lpt\ cfg-add(bin|doc|inc|lib)\ ([^ ]*)\ ([^ ]*)$ ]]; then
        echo "Ungültige Zeile: '$line'"
        exit 1
    fi
    case "${BASH_REMATCH[1]}" in
        bin) dest=lpt-bin ;;
        doc) dest=lpt-doc ;;
        inc) dest=include ;;
        lib) dest=lib ;;
        *) echo "Bug in $0, unbekanntes lpt cfg-add*"; exit 1; ;;
    esac
    echo "* system/$dest/${BASH_REMATCH[3]} -> ${BASH_REMATCH[2]}"
    mkdir -p "$2/system/$dest/"
    ln -sf "${BASH_REMATCH[2]}" "$2/system/$dest/${BASH_REMATCH[3]}"
done < "$tmpfile"

echo "* Entpacken..."
tar -C "$2" -xf "$path"
