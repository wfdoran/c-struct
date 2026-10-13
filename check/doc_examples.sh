#!/bin/sh
# Checks that the code examples in the documentation compile.
#
#   make doc-check            (or: CC=clang sh doc_examples.sh)
#
# Every ```c block of doc/*.md and README.md which includes a library header or
# defines data_t, hkey_t, value_t or prefix (an "instantiation" example) is
# compiled with -fsyntax-only.  The examples are
# written for reading, so a block passes if it compiles in any of these forms:
#
#   - as it is, with an empty main() added if it has none;
#   - with the lines after its last preprocessor line made the body of main(),
#     for the examples which show statements without a function around them;
#
# each with and without doc_prelude.h, which gives the placeholder types of the
# examples (key_type, cell_t) a meaning.  Other blocks are fragments of a larger
# program and are not compiled.
#
# Errors fail the check, and so do a call to a function that was never declared
# (the usual sign of a misspelled name), an implicit int, and a pointer or
# integer of the wrong type.  Other warnings are not shown.

CC=${CC:-gcc}
here=$(cd "$(dirname "$0")" && pwd)
inc="$here/../include"
tmp=$(mktemp -d) || exit 2
trap 'rm -rf "$tmp"' EXIT

for md in "$here"/../doc/*.md "$here"/../README.md; do
  gawk -f "$here/doc_blocks.awk" -v inc="$inc" -v outdir="$tmp" \
       -v name="$(basename "$md")" "$md" || exit 2
done

try() { # try <file> [-include doc_prelude.h]
  f=$1
  shift
  "$CC" -std=gnu11 -fsyntax-only -frounding-math -fopenmp \
        -Werror=implicit-function-declaration -Werror=implicit-int \
        -Werror=incompatible-pointer-types -Werror=int-conversion \
        -I"$inc" -I"$here" "$@" "$f" >/dev/null 2>&1
}

checked=0
failed=0
for f in "$tmp"/*.c; do
  [ -e "$f" ] || continue
  checked=$((checked + 1))

  # form 1: the block as it is
  cp "$f" "$tmp/as_is.c"
  grep -q 'int main' "$f" || printf '\nint main(void) { return 0; }\n' >> "$tmp/as_is.c"

  # form 2: the lines after the last preprocessor line inside main()
  last=$(grep -n '^[[:space:]]*#' "$f" | tail -1 | cut -d: -f1)
  last=${last:-0}
  { head -n "$last" "$f"; echo 'int main(void) {'; tail -n +"$((last + 1))" "$f"; \
    echo 'return 0;'; echo '}'; } > "$tmp/in_main.c"

  ok=0
  for form in as_is in_main; do
    try "$tmp/$form.c" -include "$here/doc_prelude.h" && ok=1 && break
    try "$tmp/$form.c" && ok=1 && break
  done

  if [ "$ok" = 0 ]; then
    failed=$((failed + 1))
    echo "FAIL: $(basename "$f" .c)  (doc block)"
    "$CC" -std=gnu11 -fsyntax-only -frounding-math -fopenmp -I"$inc" \
          -Werror=implicit-function-declaration \
          -include "$here/doc_prelude.h" "$tmp/as_is.c" 2>&1 | grep error | head -3
  fi
done

echo "doc examples: $checked checked, $failed failed"
[ "$failed" = 0 ] && [ "$checked" -gt 0 ]
