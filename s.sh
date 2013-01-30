
# (echo hello ; ls --bob ; ) && ls

(echo hello && echo good bye) > out

#(echo hello | tr e a) | (cat && echo failure)

#( cat previous_examples && cat llish.h ) | grep pid | tr aeiou ----- | cut -b 1-10 > out

# need to deal with this ONE case ... uhg
# exec echo hello
