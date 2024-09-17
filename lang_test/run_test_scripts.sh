#!/bin/bash

# Build the project
make -C ../

# Set the path to the executable
EXE=../build/lang

# Check if the executable exists
if [ ! -f "$EXE" ]; then
    echo "Error: Executable not found at $EXE"
    exit 1
fi

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


Cyan='\033[0;36m'
NC='\033[0m'
# Loop through each .ylc file and run the executable
for file in $YLC_FILES; do
    echo -e "${Green}$file:${NC}"
    echo -e "${Cyan}"
    cat $file
    echo -e "${NC}"
    
    # Run the executable and capture the output
    output=$("$EXE" "$file")
    echo -e "${Cyan}$output${NC}"
    echo "========================================="
done

# echo "Overall summary: $passed_tests/$total_tests assertions passed"

# Exit with non-zero status if any assertions failed
if [ $passed_tests -ne $total_tests ]; then
    exit 1
fi
