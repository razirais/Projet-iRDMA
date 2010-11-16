#!/bin/sh

echo "Test de chargement et dechargement du module"
echo "Et ifconfig up et down "
echo "version 0.1 -- 11/02/2010"
echo "Gaetan Harter - Oliva Adrien"
echo "-------------------------------------------------------------------"

if [ $# = 0 ]
then
	echo "Please specify path to loadable module"
	exit 1
fi

if [ ! -f $1 ]
then
	echo "Specify a real file please"
	exit 2
fi

if [ ! $(whoami) = root ]
then 
	echo "Must run as root"
	exit 3
fi

counter=10

until [ $counter = 0 ]
do
	counter=$(($counter - 1))

	# Clean kernel ring buffer
	dmesg -c > /dev/null

	#load module
	insmod "$1" > /dev/null
	if [ $? = 0 ]
	then
		echo "[$counter]Module loaded: OK"
	else
		echo "[$counter]Module loaded: FAIL"
		exit 4
	fi

	ifconfig rio0 up > /dev/null
	if [ $? = 0 ]
	then
		echo "[$counter]rio0 UP: OK"
	else
		echo "[$counter]rio0 UP: FAIL"
		exit 4
	fi

	ifconfig rio1 up > /dev/null
	if [ $? = 0 ]
	then
		echo "[$counter]rio1 UP: OK"
	else
		echo "[$counter]rio1 UP: FAIL"
		exit 4
	fi

	ifconfig rio0 down > /dev/null
	if [ $? = 0 ]
	then
		echo "[$counter]rio0 DOWN: OK"
	else
		echo "[$counter]rio0 DOWN: FAIL"
		exit 4
	fi

	ifconfig rio1 down > /dev/null
	if [ $? = 0 ]
	then
		echo "[$counter]rio1 DOWN: OK"
	else
		echo "[$counter]rio1 DOWN: FAIL"
		exit 4
	fi

	#unload module
	rmmod "$1" > /dev/null
	if [ $? = 0 ]
	then
		echo "[$counter]Module unloaded: OK"
	else
		echo "[$counter]Module unloaded: FAIL"
		exit 5
	fi
done

echo "Test passed successfully !"

