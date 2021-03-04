#!/bin/bash

echo Parse NIT:
dvbsnoop -n 1 16 | grep Network_name: | cut -d '"' -f 2
dvbsnoop -n 1 16 | grep "Center frequency:" | awk '{printf $5"\n"}'
echo ""


pids=`dvbsnoop -n 1 0 | grep Program_map_PID: | awk '{ printf $2" "}'`


for i in $pids; do
  prog_id=`dvbsnoop -n 1 0 | grep -B 3 "Program_map_PID: $i" | grep -m 1 Program_number: | awk '{printf $2}'`
  prog_name=`dvbsnoop -n 1 17 | grep -A 14 "Service_id: $prog_id" | grep Service_name: | tr -t " " "_" | cut -d '"' -f 2`

  echo PMT $i ID=$prog_id $prog_name
  ppids=`dvbsnoop -n 1 $i | grep Elementary_PID: | awk '{printf $2"\n"}'`

  for pid in $ppids; do
    d=`dvbsnoop -n 1 $i | grep -B 3 "Elementary_PID: $pid" | grep Stream_type: | awk {'printf $2'}`
    if [ $d == "2" ]; then echo " pid=$pid id=$d video";
    elif [ $d == "3" ]; then echo " pid=$pid id=$d audio";
    else echo " pid=$pid id=$d unknown";
    fi;
  done
done
