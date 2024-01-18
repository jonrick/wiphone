#!/bin/bash

file=unicodes.txt

grep  "^0" $file | sort -u > uni_0.txt

grep  "^1" $file | sort -u > uni_1.txt

grep  "^2" $file | sort -u > uni_2.txt

grep  "^3" $file | sort -u > uni_3.txt

grep  "^4" $file | sort -u > uni_4.txt

grep  "^5" $file | sort -u > uni_5.txt

grep  "^6" $file | sort -u > uni_6.txt

grep  "^7" $file | sort -u > uni_7.txt

grep  "^8" $file | sort -u > uni_8.txt

grep  "^9" $file | sort -u > uni_9.txt

grep  "^A" $file | sort -u > uni_A.txt

grep  "^B" $file | sort -u > uni_B.txt

grep  "^C" $file | sort -u > uni_C.txt

grep  "^D" $file | sort -u > uni_D.txt

grep  "^E" $file | sort -u > uni_E.txt

grep  "^F" $file | sort -u > uni_F.txt
