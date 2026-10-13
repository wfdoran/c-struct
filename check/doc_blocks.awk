#!/usr/bin/gawk -f
# Writes the ```c blocks of a markdown file which include one of the library's
# headers to files, one block per file: <outdir>/<name>_<n>.c
#
#   gawk -f doc_blocks.awk -v inc=../include -v outdir=DIR -v name=array.md array.md
#
# A block which neither includes a library header nor defines data_t, hkey_t,
# value_t or prefix is a fragment (it uses names from an earlier block, or "..."
# for code which was left out) and is skipped.
BEGIN { in_c = 0; n = 0 }
/^```c[ \t]*$/ && !in_c { in_c = 1; block = ""; uses = 0; next }
/^```/ && in_c {
  in_c = 0
  if (uses) {
    file = sprintf("%s/%s_%d.c", outdir, name, n)
    printf("%s", block) > file
    close(file)
  }
  n++
  next
}
in_c {
  block = block $0 "\n"
  # an instantiation sets one of the macros which the headers require, or
  # includes one of the headers (the macros catch a misspelled header name)
  if ($0 ~ /^[ \t]*#[ \t]*define[ \t]+(prefix|data_t|hkey_t|value_t)([ \t]|$)/) {
    uses = 1
  }
  if (match($0, /^[ \t]*#[ \t]*include[ \t]*<([A-Za-z0-9_]+\.h)>/, m)) {
    if (system("test -f " inc "/" m[1]) == 0) {
      uses = 1
    }
  }
}
