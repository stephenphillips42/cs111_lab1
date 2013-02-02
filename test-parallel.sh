#! /bin/sh

tmp=$0-$$.tmp
mkdir "$tmp" || exit

(
cd "$tmp" || exit

cat > test.sh <<'EOF'
#1
echo hello #0
#2
echo hi && (echo hello | tr eo ui | grep h) || echo error #0
#3
echo hello > file #0
#4
cat < file #1
#5
(cat | tr eo ui | grep h) < file #1
#6
cat < file ; cat < file #1
#7
echo nothing > file #2
#8
(echo hi || (echo hello && echo good bye ; (echo nothing))) #0

EOF

# Expected output varies. Just make sure it runs
# We will just make sure the word count is correct
echo "      8       8      42" > test.expwc
echo 0 > test.experrwc

../timetrash test.sh >test.out 2>test.err || exit
cat test.out | wc > test.outwc

diff -u test.expwc test.outwc || exit
) || exit
rm -fr "$tmp"



