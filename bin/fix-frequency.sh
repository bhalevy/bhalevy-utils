#!/bin/bash
file=/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor 
available=/sys/devices/system/cpu/cpu0/cpufreq/scaling_available_governors
if [ "$1" = -q ]; then
	echo "current scaling governor: `cat $file`"
	echo "available governors: `cat $available`"
	exit 0
fi
governor=performance
if [ $# -eq 1 ] ; then
    governor=$1
fi
if [ -w ${file} ] ; then
    if grep -q -- ${governor} ${available} ; then
        echo ${governor} > ${file}
    else 
        echo "${governor} is not a valid option for CPU scaling"
        echo "valid options are: $(cat $available)"
        exit 1
    fi
else
    echo "You do not have permission to write ${file}"
    exit 1
fi
exit 0


