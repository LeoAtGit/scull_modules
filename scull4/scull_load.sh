#!/bin/bash

module="scull"

/sbin/insmod ./$module.ko || exit 1

major=$(awk -f insert_scull.awk /proc/devices)

rm -f /dev/$module

mknod /dev/$module c $major 0
