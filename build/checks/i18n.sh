#!/usr/bin/env bash
# Every user-facing string is English in the source and passes through tr() (or is marked
# N_() where it is defined and translated where shown). This checks src/core/i18n_es.c
# against the code: each such string has a Spanish entry with the same printf conversions,
# and no entry is left unused or duplicated.
#   build/checks/i18n.sh            check
#   build/checks/i18n.sh --missing  print the strings with no entry, as table lines
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/../.."
perl -e '
  use strict; use warnings;
  my $missing_mode = ($ARGV[0] // "") eq "--missing";
  my $lit = qr/"(?:[^"\\]|\\.)*"/;
  sub join_lits { my $s = shift; my $o = ""; $o .= $1 while $s =~ /"((?:[^"\\]|\\.)*)"/g; $o }
  sub convs { my $s = shift; $s =~ s/%%//g; join(" ", $s =~ /%[-+ #0]*\d*(?:\.\d+)?(?:hh|h|ll|l|z)?[diouxXcsp]/g) }
  my (%used, %where);
  for my $f (split /\n/, `find src -name "*.[ch]" ! -name i18n_es.c ! -name i18n.h | sort`) {
    open my $fh, "<", $f or die; local $/; my $c = <$fh>;
    $c =~ s{/\*.*?\*/}{ }gs;                       # comments may quote strings
    while ($c =~ /\b(?:tr|N_)\(\s*((?:$lit\s*)+)\)/g) { my $k = join_lits($1); $used{$k} = 1; $where{$k} //= $f }
  }
  open my $t, "<", "src/core/i18n_es.c" or die; local $/; my $tab = <$t>;
  my (%table, $fail);
  while ($tab =~ /\{\s*((?:$lit\s*)+),\s*((?:$lit\s*)+)\}/g) {
    my ($en, $es) = (join_lits($1), join_lits($2));
    if (exists $table{$en}) { print STDERR "check-i18n: duplicated entry: \"$en\"\n"; $fail = 1 }
    $table{$en} = $es;
    if (convs($en) ne convs($es)) { print STDERR "check-i18n: printf conversions differ: \"$en\"\n"; $fail = 1 }
  }
  for my $k (sort keys %used) {
    next if exists $table{$k};
    if ($missing_mode) { print "    {\"$k\",\n     \"\"},\n" } else { print STDERR "check-i18n: no Spanish for \"$k\" ($where{$k})\n"; $fail = 1 }
  }
  for my $k (sort keys %table) {
    next if $used{$k} || $missing_mode;
    print STDERR "check-i18n: unused entry: \"$k\"\n"; $fail = 1;
  }
  exit($fail ? 1 : 0);
' -- "${1:-}"
