#!/bin/bash

generate_xml() {
    local dir="$1"
    local indent="$2"

    echo "$indent<folder Name=\"$(basename "$dir")\">"

    for entry in "$dir"/*; do
        if [ -d "$entry" ]; then
            generate_xml "$entry" "$indent  "
        else
            echo "$indent  <file file_name=\"$(realpath "$entry")\" />"
        fi
    done

    echo "$indent</folder>"
}

# Set the root directory
root_directory="./lvgl/src"

# Call the function to generate XML-like structure
generate_xml "$root_directory" ""
