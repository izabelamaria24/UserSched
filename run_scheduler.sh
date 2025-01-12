#!/bin/bash

TARGET="scheduler"
SRC="scheduler_multicore.c"
LOG="logging.txt"

if ! command -v gcc &> /dev/null; then
  echo "gcc could not be found. Please install it and try again."
  exit 1
fi

if [[ "$OSTYPE" != "linux-gnu"* && "$OSTYPE" != "darwin"* ]]; then
    echo "This script is designed for Linux/macOS. Exiting."
    exit 1
fi

echo "Compiling $SRC..."
gcc -pthread -Wall -Wextra -g -o $TARGET $SRC
if [ $? -ne 0 ]; then
  echo "Compilation failed. Exiting."
  exit 1
fi
echo "Compilation successful."

echo "Running $TARGET..."
./$TARGET

echo "Cleaning up..."
rm -f $TARGET $LOG
echo "Done."
