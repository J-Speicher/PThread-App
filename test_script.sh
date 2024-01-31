#!/bin/bash

# Check if the correct number of arguments is provided
if [ "$#" -ne 1 ]; then
    echo "Usage: bash test_script.sh number_of_threads"
    exit 1
fi

# Set the command and number of threads
par_sum="./par_sum"
number_of_threads="$1"

# Loop through each file in the "tests" folder
for file in tests/*; do
    # Check if the file is a regular file
    if [ -f "$file" ]; then
        echo "Running $file with $number_of_threads threads..."
        
        # Measure the time it takes to run the command
        start_time=$(date +%s.%N)
        
        # Execute the binary file par_sum with the arguments
        $par_sum "$file" "$number_of_threads"
        
        # Wait for par_sum to finish running
        wait
        
        end_time=$(date +%s.%N)
        
        # Calculate the elapsed time
        elapsed_time=$(echo "$end_time - $start_time" | bc)
        
        echo "Time taken: $elapsed_time seconds"
        echo
    fi
done