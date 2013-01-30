#echo hi > out < s.sh

#(echo hello ; ls --bob ; ) && ls

(echo hello | tr e a) | (cat - && echo failure)

#(echo hello | (cat))

#( cat previous_examples && cat llish.h ) | grep pid | tr aeiou ----- | cut -b 1-10 

# need to deal with this ONE case ... uhg
# exec echo hello
