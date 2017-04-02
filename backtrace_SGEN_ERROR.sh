#!/bin/bash

for addr in $(./sgen input_test.sg 2>&1 | grep './sgen()' | awk '{ print $2 }' | tr -d '[]'); do addr2line -f -e sgen $addr && echo ""; done
