#!/bin/bash
sleep 5
DIRECTORY="/sys/devices/platform/ir.0"	

if [ ! -d "$DIRECTORY" ];
then 
	insmod /home/pi/LDD/A2/ir-device/ir-dev.ko

fi


FLAG=0 #FLAG = 1 represents that HDMI is connected now. 
while true; do

	while raspi-gpio get 46 | grep level=0; do
		#HDMI is Connected here
		FLAG=1
		echo $FLAG
		sleep 2
	done
	echo "HDMI got disconnected"
	echo $FLAG

	if [ $FLAG == 1 ] 
	then
		echo "inside if"
		echo 1 > /sys/devices/platform/ir.0/status
		sleep 2
	fi

	while raspi-gpio get 46 | grep level=1; do
		#HDMI is disconnected
		echo "HDMI is disconnected"
		echo $FLAG
		sleep 2
	done

	echo 1 > /sys/devices/platform/ir.0/status
		
sleep 2
done
