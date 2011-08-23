#!/bin/sh

WORKDIR=$1

if [ ! ${WORKDIR} ]; then
	WORKDIR="./"
fi

DROP_NAME="scale2x-2.4"
FILE_NAME="scale2x-2.4.tar.gz"
FILE_URL="http://downloads.sourceforge.net/scale2x/${FILE_NAME}?download"

if [ ! -d ${WORKDIR}${DROP_NAME} ]; then

	if [ ! -f ${WORKDIR}${FILE_NAME} ]; then
		wget --quiet ${FILE_URL} -O "${WORKDIR}${FILE_NAME}" \
			|| (echo "[$DROP_NAME] Unable to download $FILE_NAME from $FILE_URL" && rm ${WORKDIR}${FILE_NAME}) 
	fi

	if [ ! -f ${WORKDIR}${FILE_NAME} ]; then
		exit 1
		#return 1
	fi

	tar xzf ${WORKDIR}${FILE_NAME} --directory ${WORKDIR}

fi