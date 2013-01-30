#! /bin/sh

tmp=$0-$$.tmp
mkdir "$tmp" || exit

echo "$tmp"

(
cd "$tmp" || exit

pwd

cat > test.sh <<'EOF'
#echo hello

#echo hello world i have ice cream | grep h

#(echo hello world || echo fail)

#(echo hello world && echo fail)

#(echo hello world && echo fail) > test.tmp

#(echo hello world && echo fail) > test.tmp | cat ./test.tmp

(echo hello world | tr eo oe) | (cat - | cut -b 1-4 && echo finished)

#echo hello my world is falling apart > test2.tmp

#cat < test2.tmp | tr aeiou -----

EOF

cat > test.exp <<'EOF'
hello
hello world i have ice cream
hello world
hello world
fail
hello world
fail
holl
finished
h-ll- my w-rld -s f-ll-ng -p-rt
EOF

../timetrash test.sh >test.out 2>test.err || exit

diff -u test.exp test.out || exit
test ! -s test.err || {
  cat test.err
  exit 1
}

) || exit

rm -fr "$tmp"



