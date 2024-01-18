Two scripts to get the unicodes in a sorted way without repeat to any unicode

1- getunicode.sh script:
   a- it reads the spread sheet of japanese strings line by line and extract the japanese strings
   b- it get the unicode for each character
   
   This scrit should be run and the output redirected to a file as following
   ./extract_sorted_unicodes.sh > unicodes.txt
   
   
2- extract_sorted_unicodes.sh script:
   a- it reads the output file of getunicodes.sh 
   b- it arrange the unicodes according to the begin byte, it arranges them in files. all the unicodes begins with zero in one file and same for [1:f]
   c- it sorted the unicodes so no repeatition
   
The output files of this script should be used in the PDE script of creating the smooth font

