#!/bin/sh

. ../../../../HTML5/Build_All_HTML5_libs.rc


cd ../..

proj=libpng
makefile=projects/HTML5/Makefile.HTML5

make clean   OPTIMIZATION=-O3 LIB=${proj}_O3.bc -f ${makefile}
make install OPTIMIZATION=-O3 LIB=${proj}_O3.bc -f ${makefile}

make clean   OPTIMIZATION=-O2 LIB=${proj}_O2.bc -f ${makefile}
make install OPTIMIZATION=-O2 LIB=${proj}_O2.bc -f ${makefile}

make clean   OPTIMIZATION=-Oz LIB=${proj}_Oz.bc -f ${makefile}
make install OPTIMIZATION=-Oz LIB=${proj}_Oz.bc -f ${makefile}

make clean   OPTIMIZATION=-O0 LIB=${proj}.bc -f ${makefile}
make install OPTIMIZATION=-O0 LIB=${proj}.bc -f ${makefile}

ls -l lib/HTML5

