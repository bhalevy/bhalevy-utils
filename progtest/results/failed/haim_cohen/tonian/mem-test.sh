#!/bin/sh
valgrind --leak-check=full --leak-resolution=high --show-reachable=yes --error-limit=no --undef-value-errors=no ./tonian
