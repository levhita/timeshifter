#!/bin/bash
usage="Compile and Installs Timeshifter

Usage: install.sh [-options]

Where options can be
-c	Compile timeshifter_core
-u	Copy the executables to ~/.bin (run it again when updating to a new version)
-e 	Adds the export line at the end of the .bashrc sou you can run it everywhere"

hflag='yes'
cflag='no'
uflag='no'
eflag='no'
#Go to your home directory and append the export line to .bashrc.
while getopts "cue" opt; do
	case $opt in
		c)
			hflag='no'
			cflag='yes'
			;;
		u)	
			hflag='no'
			uflag='yes'
			;;

		e)	
			hflag='no'
			eflag='yes'
			;;
	esac
done

shift $(expr $OPTIND - 1 )

if [ $hflag = yes ]; then
	echo "$usage">&2
	exit 1
fi
if [ $cflag = yes ]; then
	gcc main.c -lpng -o timeshifter_core
fi

if [ $uflag = yes ]; then
	if [ ! -f timeshifter_core  ]; then
		echo "timeshifter_core hasn't been compiled, please run the installer again with the -c flag">&2
		exit 1
	fi	
	#Make a .bin dir in your home and copy timeshifter on it.
	if [ ! -d ~/.bin  ]; then
		mkdir ~/.bin 
	fi
	cp timeshifter ~/.bin/
	cp timeshifter_core ~/.bin/
fi

if [ $eflag = yes ]; then
	cd ~
	echo 'export PATH=$PATH:$HOME/.bin/' >> .bashrc
fi


