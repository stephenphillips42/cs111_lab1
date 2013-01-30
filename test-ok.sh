#! /bin/sh

echo hello

ls

echo hello world i have ice cream | grep h

(echo hello world || echo fail)

(echo hello world && echo fail)

(echo hello world && echo fail) > test.txt

(echo hello world && echo fail) > test.txt | cat ./test.txt
