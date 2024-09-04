#!/bin/bash

# Build the project
make -C ../

# Set the path to the executable
EXE=../build/audio_lang

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
YLC_FILES=$(find ./test_scripts -name "*.ylc")

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

# Loop through each .ylc file and run the executable
for file in $YLC_FILES; do
    echo "Processing file: $file"
    
    # Extract all assertions from the file
    assertions=$(grep "# %assert" "$file" | sed "s/.*# %assert '\(.*\)'/\1/")
    
    if [ -z "$assertions" ]; then
        echo "Warning: No assertions found in $file"
        continue
    fi

    # Run the executable and capture the output
    output=$("$EXE" "$file")
    echo "$output"
    
    # Filter out lines starting with "> " and split remaining output into lines
    filtered_output=$(echo "$output" | grep -v "^> ")
    IFS=$'\n' read -d '' -r -a output_lines <<< "$filtered_output"
    
    # Counter for assertions in this file
    file_assertions=0
    file_passed=0
    output_line_index=0
    
    # Check each assertion
    while IFS= read -r expected_output; do
        file_assertions=$((file_assertions + 1))
        total_tests=$((total_tests + 1))
        
        # Get the next non-empty line from the output
        actual_output=""
        while [ -z "$actual_output" ] && [ $output_line_index -lt ${#output_lines[@]} ]; do
            actual_output=$(trim "${output_lines[output_line_index]}")
            output_line_index=$((output_line_index + 1))
        done
        
        expected_output=$(trim "$expected_output")
        
        echo "Assertion $file_assertions: '$expected_output'"
        echo "Actual output: '$actual_output'"
        
        if [ "$actual_output" == "$expected_output" ]; then
            echo "✅ PASSED"
            file_passed=$((file_passed + 1))
            passed_tests=$((passed_tests + 1))
        else
            echo "❌ FAILED"
        fi
        echo "-------------------------"
    done <<< "$assertions"
    
    echo "File summary: $file_passed/$file_assertions assertions passed"
    echo "========================================="
done

echo "Overall summary: $passed_tests/$total_tests assertions passed"

# Exit with non-zero status if any assertions failed
if [ $passed_tests -ne $total_tests ]; then
    exit 1
fi
