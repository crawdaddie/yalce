#!/bin/bash

# Build the project
make -C ../

# Set the path to the executable
EXE=ylc --test

# Check if the test_scripts directory exists
if [ ! -d "./test_scripts" ]; then
    echo "Error: test_scripts directory not found"
    exit 1
fi

# Find all .ylc files in the test_scripts directory
YLC_FILES=$(find ./test_scripts -name "*.ylc" | sort)

# Check if any .ylc files were found
if [ -z "$YLC_FILES" ]; then
    echo "No .ylc files found in ./test_scripts"
    exit 1
fi

# Initialize counters
total_tests=0
passed_tests=0

Red='\033[0;31m'
Green='\033[0;32m'
Yellow='\033[0;33m'
Blue='\033[0;34m'
Purple='\033[0;35m'
Cyan='\033[0;36m'
NC='\033[0m'

# Loop through each .ylc file and run the executable
for file in $YLC_FILES; do
    ylc --test $file
done

