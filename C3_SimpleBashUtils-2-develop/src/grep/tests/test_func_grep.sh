#!/bin/bash

SUCCESS=0
FAIL=0
COUNTER=0
DIFF_RES=""


declare -a tests=(
"VAR for test_1_grep.txt test_2_grep.txt"
"VAR _ test_1_grep.txt test_2_grep.txt"
"VAR aboba  test_2_grep.txt"
"VAR int test_5_grep.txt"
"VAR king test_1_grep.txt"
"VAR ^\} test_4_grep.txt"
"VAR g test_0_grep.txt"
"VAR 52 test_2_grep.txt"
"VAR 163 test_1_grep.txt"
)
declare -a extra=(
"-n for test_1_grep.txt test_2_grep.txt"
"-n for test_1_grep.txt"
"-n -e ^\} test_1_grep.txt"
"-c -e / test_1_grep.txt"
"-c ^int test_1_grep.txt test_2_grep.txt"
"-e ^int test_1_grep.txt"
"-n -i = test_1_grep.txt test_2_grep.txt"
"-c test_1_grep.txt test_2_grep.txt"
"-c -l aboba test_1_grep.txt test_5_grep.txt"
"-v -e ank test_1_grep.txt"
"-n ) test_5_grep.txt"
"-l for test_1_grep.txt test_2_grep.txt"
"-e int test_4_grep.txt"
"-e = -e out test_5_grep.txt"
"-n ing -e as -e the -e not -e is test_6_grep.txt"
"-e ing -e as -e the -e not -e is test_6_grep.txt"
"-c -e . -e '.' test_1_grep.txt"
"-l for no_file.txt test_2_grep.txt"
"t test_1_grep.txt"
"-c aboba test_1_grep.txt test_2_grep.txt test_3_grep.txt"
"-v -e ank -c test_1_grep.txt"
"-e"
"-aas asd test_1_grep.txt"
""
)

testing(){
    t=$(echo $@ | sed "s/VAR/$var/")
    ./s21_grep $t > test_s21_grep.log
    grep $t > test_sys_grep.log
    DIFF_RES="$(diff -s test_s21_grep.log test_sys_grep.log)"
    (( COUNTER++ ))
    if [ "$DIFF_RES" == "Files test_s21_grep.log and test_sys_grep.log are identical" ]
    then
      (( SUCCESS++ ))
      echo "\033[31m$FAIL\033[0m/\033[32m$SUCCESS\033[0m/$COUNTER \033[32msuccess\033[0m grep $t"
    else
      (( FAIL++ ))
      echo "\033[31m$FAIL\033[0m/\033[32m$SUCCESS\033[0m/$COUNTER \033[31mfail\033[0m grep $t"
    fi
    rm test_s21_grep.log test_sys_grep.log
}

# тесты для extra
for i in "${extra[@]}"
do
    var="-"
    testing $i
done

# С 1 параметром
for var1 in v c l n
do
    for i in "${tests[@]}"
    do
        var="-$var1"
        testing $i
    done
done

# С 2 параметрами
for var1 in v c l n
do
    for var2 in v c l n
    do
        if [ $var1 != $var2 ]
        then
            for i in "${tests[@]}"
            do
                var="-$var1 -$var2"
                testing $i
            done
        fi
    done
done

# С 3 параметрами
for var1 in v c l n
do
    for var2 in v c l n
    do
        for var3 in v c l n
        do
            if [ $var1 != $var2 ] && [ $var2 != $var3 ] && [ $var1 != $var3 ]
            then
                for i in "${tests[@]}"
                do
                    var="-$var1 -$var2 -$var3"
                    testing $i
                done
            fi
        done
    done
done

echo "\033[31mFAIL: $FAIL\033[0m"
echo "\033[32mSUCCESS: $SUCCESS\033[0m"
echo "ALL: $COUNTER"
