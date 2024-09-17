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

# Initialize counters
total_tests=0
passed_tests=0

# Function to trim whitespace from a string
trim() {
    local var="$*"
    var="${var#"${var%%[![:space:]]*}"}"
    var="${var%"${var##*[![:space:]]}"}"   
    echo -n "$var"
}
Red='\033[0;31m'
Green='\033[0;32m'
Yellow='\033[0;33m'
Blue='\033[0;34m'
Purple='\033[0;35m'
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
    
    # Filter out lines starting with "> " and split remaining output into lines
#     filtered_output=$(echo "$output" | grep -v "^> ")
#     IFS=$'\n' read -d '' -r -a output_lines <<< "$filtered_output"
#     
#     # Counter for assertions in this file
#     file_assertions=0
#     file_passed=0
#     output_line_index=0
#     
#     # Check each assertion
#     while IFS= read -r expected_output; do
#         file_assertions=$((file_assertions + 1))
#         total_tests=$((total_tests + 1))
#         
#         # Get the next non-empty line from the output
#         actual_output=""
#         while [ -z "$actual_output" ] && [ $output_line_index -lt ${#output_lines[@]} ]; do
#             actual_output=$(trim "${output_lines[output_line_index]}")
#             output_line_index=$((output_line_index + 1))
#         done
#         
#         expected_output=$(trim "$expected_output")
#         
#         echo "Assertion $file_assertions: '$expected_output'"
#         
#         if [ "$actual_output" == "$expected_output" ]; then
#             echo -e "✅${Green}PASSED${NC}"
#             file_passed=$((file_passed + 1))
#             passed_tests=$((passed_tests + 1))
#         else
#             echo -e "❌${Red}FAILED${NC}"
#             echo -e "Actual output:${Purple}
# $output${NC}"
#         fi
#         echo "-------------------------"
#     done <<< "$assertions"
    
    echo "File summary: $file_passed/$file_assertions assertions passed"
    echo "========================================="
done

echo "Overall summary: $passed_tests/$total_tests assertions passed"

# Exit with non-zero status if any assertions failed
if [ $passed_tests -ne $total_tests ]; then
    exit 1
fi
