#!/bin/bash



if [ "$#" -ne 2 ]; then
    echo "Usage: ./split_pgn.sh <input_pgn_file> <output_directory>"
    exit 1
fi

file="$1"
output_dir="$2"
filename=$(basename "$file" .pgn)
empty_lines=0
part_number=1
current_block=""

# Check if input file exists
if [ ! -f "$file" ]; then
    echo "Error: File '$file' does not exist."
    exit 1
fi

# Create the output directory if it doesn't exist
mkdir -p "$output_dir"
last_folder=$(basename "$output_dir")

while IFS= read -r line; do
    current_block+="$line"$'\n'
    
    if [ -z "$line" ]; then
        empty_lines=$((empty_lines + 1))
    fi

    if [ $((empty_lines % 2)) -eq 0 ] && [ $empty_lines -ne 0 ]; then
        echo "$current_block" > "$output_dir/${filename}_$part_number.pgn"
        echo "Saved game to $last_folder/${filename}_$part_number.pgn"
        part_number=$((part_number + 1))
        current_block=""
        empty_lines=0
    fi
done < "$file"

# Save any remaining block if it exists
if [ -n "$current_block" ]; then
    echo "$current_block" > "$output_dir/${filename}_$part_number.pgn"
    echo "Saved game to $last_folder/${filename}_$part_number.pgn"
fi

echo "All games have been split and saved to '$last_folder'." 
