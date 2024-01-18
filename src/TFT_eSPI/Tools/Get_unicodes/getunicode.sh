#!/bin/bash

#spread sheet of japanese strings
localFile=trans_jap.csv

while IFS= read -r line
do
  var=$(echo "$line" | awk -F ',' '{print $2}') 
  uni=$(echo "$var" | tr -d '"') 
  echo "$uni"
  unicode --max 0 --brief -s $uni | awk '{print $2}' | cut -b 3-
done < $localFile



