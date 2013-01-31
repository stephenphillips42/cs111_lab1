#! /bin/sh

tmp=$0-$$.tmp
mkdir "$tmp" || exit

(
cd "$tmp" || exit

cat > test.sh <<'EOF'
echo hello
echo 1
echo hello world i have ice cream | grep h
echo 2
(echo hello world || echo fail)
echo 3
(echo hello world && echo fail)
echo 4
echo hello world || echo --badbadbad
echo 5
cat --notanarg || echo could not cat
echo 6
echo hello world && echo --badbadbad || echo nothing to it
echo 7
(echo hello world && echo fail) > test.tmp
echo 8
(echo hello world && echo fail) < test.tmp | cat ./test.tmp
echo 9
(echo hello world | tr eo oe) | (cat - | cut -b 1-4 && echo finished)
echo 10
echo hello my world is falling apart > test2.tmp
echo 11
cat < test2.tmp | tr aeiou -----
echo 12
(echo hello ; ls --bob ; ) && ls
echo 13
(echo hello | tr e a) | (cat && echo failure)
echo 14
( cat previous_examples && cat llish.h ) | grep pid | tr aeiou ----- | cut -b 1-10 
echo 15
cat < test2.tmp | (cat - | grep is | (echo hello)) | tr aeiou -----
echo 16
(echo this is good; cat --notagain) || echo subshell has not succeeded in his mission
EOF

bash test.sh > test.exp 2>test.experr

../timetrash test.sh >test.out 2>test.err || exit

diff -u test.exp test.out || exit
diff -u test.experr test.err || exit

) || echo LAST ALMOST WHY && exit

rm -fr "$tmp"



