#!/bin/sh

WORKDIR=$1

if [ ! ${WORKDIR} ]; then
	WORKDIR="./"
fi


test_file()
{
	LAST_NAME=$1

	if [ ! -f ${WORKDIR}${LAST_NAME} ]; then

		fetch_drop $2 $3 $4
		
		if [ $! ]; then
			return 1
		fi 

		${INST_CALLBACK}

		if [ ! -f ${WORKDIR}${LAST_NAME} ]; then
			return 1
		fi

	fi
	return 0
}

fetch_drop()
{
	DROP_NAME=$1
	FILE_NAME=$2
	FILE_URL=$3

	if [ ! -d ${WORKDIR}${DROP_NAME} ]; then
	
		if [ ! -f ${WORKDIR}${FILE_NAME} ]; then

			${DOWN_CALLBACK}

			if [ $! ]; then
				return 1
			fi

		fi

	fi
	return 0
}

install_scale2x()
{
	tar xzf ${WORKDIR}${FILE_NAME} --directory ${WORKDIR}

	cp ${WORKDIR}${DROP_NAME}/contrib/sdl/scale2x.c ${WORKDIR}${LAST_NAME}
}
download_wget()
{
	wget --quiet ${FILE_URL} -O "${WORKDIR}${FILE_NAME}" \
		|| (echo "[$DROP_NAME] Unable to download $FILE_NAME from $FILE_URL" && rm ${WORKDIR}${FILE_NAME}) 


	if [ ! -f ${WORKDIR}${FILE_NAME} ]; then
		return 1
	fi
}


install_inprint()
{
	#FONT_FILE=../data/free/openkb8x8.bmp
	#cp ${WORKDIR}SDL_inprint/inprint.c ${WORKDIR}${LAST_NAME}
	#XBM="php ${WORKDIR}SDL_inprint/bmp2xbm.php"
	#echo "#include <SDL.h>" > tmp1.c
	#${XBM} ${FONT_FILE} >> tmp1.c
	#cat inprint.c > tmp2.c
	#sed '/#include/d' tmp2.c >> tmp1.c
	sed 's/inline_font\.h/..\/src\/font.h/' ${WORKDIR}${DROP_NAME}/inprint.c > ${WORKDIR}${LAST_NAME}
	#rm tmp2.c
	#mv tmp1.c inprint.c
}

download_git()
{
	git clone ${FILE_URL} ${WORKDIR}${DROP_NAME}
}

install_sha256()
{
	tar xzf ${WORKDIR}${FILE_NAME} --directory ${WORKDIR}

	cp ${WORKDIR}${DROP_NAME}/${LAST_NAME} ${WORKDIR}${LAST_NAME}
#	cp ${WORKDIR}${DROP_NAME}/sha2-1.0.1/sha2.h ${WORKDIR}sha2.h
}

install_savepng()
{
    echo "#ifdef HAVE_LIBPNG" > ${WORKDIR}savepng.c
    echo "#ifdef HAVE_LIBPNG" > ${WORKDIR}savepng.h
    cat ${WORKDIR}${DROP_NAME}/savepng.c >> ${WORKDIR}${LAST_NAME}
    cat ${WORKDIR}${DROP_NAME}/savepng.h >> ${WORKDIR}savepng.h
    echo "#endif /* HAVE_LIBPNG */" >> ${WORKDIR}savepng.c
    echo "#endif /* HAVE_LIBPNG */" >> ${WORKDIR}savepng.h
}

REMOTE_NAME=scale2x-2.4
REMOTE_FILE=${REMOTE_NAME}.tar.gz
REMOTE_URL="http://downloads.sourceforge.net/scale2x/${REMOTE_FILE}?download"
DOWN_CALLBACK=download_wget
INST_CALLBACK=install_scale2x
test_file "scale2x.c" "${REMOTE_NAME}" "${REMOTE_FILE}" "${REMOTE_URL}" 

REMOTE_NAME=SDL_inprint
REMOTE_FILE=SDL_inprint
REMOTE_URL="git://github.com/driedfruit/${REMOTE_NAME}.git"
DOWN_CALLBACK=download_git
INST_CALLBACK=install_inprint
test_file "inprint.c" "${REMOTE_NAME}" "${REMOTE_FILE}" "${REMOTE_URL}" 

REMOTE_NAME=SDL_SavePNG
REMOTE_FILE=SDL_SavePNG
REMOTE_URL="git://github.com/driedfruit/${REMOTE_NAME}.git"
DOWN_CALLBACK=download_git
INST_CALLBACK=install_savepng
test_file "savepng.c" "${REMOTE_NAME}" "${REMOTE_FILE}" "${REMOTE_URL}"
test_file "savepng.h" "${REMOTE_NAME}" "${REMOTE_FILE}" "${REMOTE_URL}"

REMOTE_NAME=sha2-1.0.1
REMOTE_FILE=${REMOTE_NAME}.tgz
REMOTE_URL="http://www.aarongifford.com/computers/${REMOTE_FILE}"
DOWN_CALLBACK=download_wget
INST_CALLBACK=install_sha256
test_file "sha2.c" "${REMOTE_NAME}" "${REMOTE_FILE}" "${REMOTE_URL}"
test_file "sha2.h" "${REMOTE_NAME}" "${REMOTE_FILE}" "${REMOTE_URL}"

exit 0
