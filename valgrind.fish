for file in (ls tests/*.in | sed 's/\.in//g' | sed 's-tests/--g')

    valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes --verbose --error-exitcode=10 \
        ./main <tests/$file.in &> $file.valgrind

    set result $status
    if test $result -eq 10
        echo $file: MEMORY PROBLEM
    else if test $result -ne 0
        echo $file: UNEXPECTED ERROR $result
    else
        rm $file.valgrind
        echo $file: OK
    end
end
