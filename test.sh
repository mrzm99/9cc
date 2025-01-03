#!/bin/bash
assert() {
    expected="$1"
    input="$2"

    ./9cc "$input" > tmp.s
    cc -o tmp tmp.s
    ./tmp
    actual="$?"

    if [ "$actual" = "$expected" ]; then
        echo "$input => $actual"
    else
        echo "$input => $expected expected, but got $actual"
        exit 1
    fi
}

assert 0 "0;"
assert 42 "42;"
assert 21 "5+20-4;"
assert 41 " 12 + 34 - 5; "
assert 47 "5+6*7;"
assert 15 "5*(9-6);"
assert 4 "(3+5)/2;"
assert 12 "-10+22;"
assert 7 "+2+-2-9+16;"
assert 10 "-10*(-2)-30++20;"
assert 1 "+100==100;"
assert 1 "-192==-192;"
assert 1 "1 < 100;"
assert 0 "+39 <= -190;"
assert 1 "3 > -90;"
assert 0 "-30 >= +20;"
assert 1 "a=1;"
assert 2 "a=1;b=2;"
assert 99 "a=b=99;"
assert 11 "a=1;b=c=11;"
assert 10 "b=1+1+1+7;a=b;"
assert 0 "a = -9;c = a++1 + 2 *4;"
assert 1 "fogefoge = 1;"
assert 99 "foge=30; piyo=70; foobar = piyo +foge -1;"

echo OK