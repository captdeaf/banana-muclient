#!/bin/bash

read file
file -i "$file" | perl -p -e 's/.*:\s+//'
