#! /bin/sh

tmp=$0-$$.tmp
mkdir "$tmp" || exit

(
cd "$tmp" || exit

cat > test.sh <<'EOF'
echo hello

echo hello world i have ice cream | grep h

(echo hello world || echo fail)

(echo hello world && echo fail)

echo hello world || echo --badbadbad

echo hello world && echo --badbadbad || echo nothing to it

#(echo hello world && echo fail) > test.tmp

#(echo hello world && echo fail) > test.tmp | cat ./test.tmp

(echo hello world | tr eo oe) | (cat - | cut -b 1-4 && echo finished)

echo hello my world is falling apart > test2.tmp

cat < test2.tmp | tr aeiou -----

(echo hello ; ls --bob ; ) && ls

(echo hello | tr e a) | (cat && echo failure)

( cat previous_examples && cat llish.h ) | grep pid | tr aeiou ----- | cut -b 1-10 

EOF
bash test.sh > test.exp

../timetrash test.sh >test.out 2>test.err || exit

diff -u test.exp test.out || exit
test ! -s test.err || {
  cat test.err
  exit 1
}

) || exit

rm -fr "$tmp"



