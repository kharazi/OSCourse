#! /bin/bash

grep -E --color -o '[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}' $1 | sort | uniq | wc -l

