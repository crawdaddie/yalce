#! /bin/bash

file=$1
expects=$(grep -oE 'expect: .+$' $file | cut -c 9-)
echo "$expects" > expected
./lang $file > actual 2>&1 # capture stderr to to file as well as stdout
result=$(diff --color=always actual expected)
if [ -z "$result" ];
then printf "\e[0;32m\xE2\x9C\x94\e[0m $file\n"
else
printf "\e[0;31m\xE2\x9C\x98\e[0m  $file\n"
fi
echo "$result"

# rm actual expected
