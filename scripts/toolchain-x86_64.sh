export BINUTILS_VERSION=2.30
export GCC_VERSION=7.3.0

export GNU_MIRROR_BASE=https://ftp.gnu.org/gnu
export ISL_MIRROR_BASE=http://isl.gforge.inria.fr
export CLOOG_MIRROR_BASE=https://syd.sh/stuff

export PATH=$HOME/tools:$PATH

function download_compile {
	echo $1
	curl $1 > $2.archive
	tar -xf $2.archive
	mkdir $2-build
	cd $2-build
	../$2/configure --prefix=$HOME/tools $3
	if [ "$2" == "gcc-$GCC_VERSION" ]; then
		make all-gcc -j$(getconf _NPROCESSORS_ONLN)
		make all-target-libgcc -j$(getconf _NPROCESSORS_ONLN)
		make install-gcc
		make install-target-libgcc
	else
		make -j$(getconf _NPROCESSORS_ONLN)
		make install
	fi
	cd ..
	rm -rf $2*
}

set -e
download_compile $GNU_MIRROR_BASE/binutils/binutils-$BINUTILS_VERSION.tar.gz binutils-$BINUTILS_VERSION "--with-isl=$HOME/tools --with-cloog=$HOME/tools --with-sysroot --disable-nls --disable-werror --target=x86_64-elf"
download_compile $GNU_MIRROR_BASE/gcc/gcc-$GCC_VERSION/gcc-$GCC_VERSION.tar.gz gcc-$GCC_VERSION "--with-isl=$HOME/tools --with-cloog=$HOME/tools --with-gmp=$HOME/tools --with-mpfr=$HOME/tools --with-mpc=$HOME/tools --disable-nls --enable-languages=c,c++ --without-headers --target=x86_64-elf"