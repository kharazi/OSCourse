#!/bin/bash
 
a=(1 2 2 1)
b=(3 1 1 2)
 
grep -f <(printf "%s\n" "${a[@]}") <(printf "%s\n" "${b[@]}")|uniq
 
awk 'FNR==NR{a[$0];next} $0 in a && !($0 in b){b[$0]} END{for (i in b) print i}' <(printf "%s\n" "${a[@]}")  <(printf "%s\n" "${b[@]}")