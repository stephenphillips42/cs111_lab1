echo hello

echo hi && (echo hello | tr eo ui | grep h) || echo error

echo hello > file

cat < file

(cat | tr eo ui | grep h) < file

echo nothing > file

(echo hi || (echo hello && echo good bye ; (echo nothing)))




# need to deal with this ONE case ... uhg
# exec echo hello
