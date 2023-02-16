#! /bin/bash

file=$1
expects=$(grep -oE 'expect: .+$' $file | cut -c 9-)
echo "$expects" > expected
echo "$(./lang $file)" > actual
result=$(diff actual expected)
if [ -z "$result" ];
then printf "\e[0;32m\xE2\x9C\x94\e[0m $file\n"
else
printf "\e[0;31m\xE2\x9C\x98\e[0m  $file\n"
fi
echo "$result"

rm actual expected
