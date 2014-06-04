#! /bin/bash

c=0
for i in `grep -E -o '[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}' log.txt| sort | uniq`
do
    loc=`geoiplookup $i`    
    if [ "$loc" == "GeoIP Country Edition: US, United States" ]; then 
        echo $i
        let c+=1
    fi
done
echo $c
