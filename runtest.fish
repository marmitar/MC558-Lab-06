for file in (ls tests/*.in | sed 's/\.in//g' | sed 's-tests/--g')
    ./main < tests/$file.in diff | diff -qs - tests/$file.out
end
