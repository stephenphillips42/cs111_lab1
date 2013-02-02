#1
echo hello #0
#2
echo hi && (echo hello | tr eo ui | grep h) || echo error #0
#3
echo hello > file #0
#4
cat < file #1
#5
(cat | tr eo ui | grep h) < file #2
#6
cat < file ; cat < file #2
#7
echo nothing > file #3
#8
(echo hi || (echo hello && echo good bye ; (echo nothing))) #0




# need to deal with this ONE case ... uhg
# exec echo hello
