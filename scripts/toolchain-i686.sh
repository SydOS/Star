export GMP_VERSION=6.1.2
export MPFR_VERSION=4.0.1
export MPC_VERSION=1.1.0
export ISL_VERSION=0.19
export CLOOG_VERSION=0.18.4
export BINUTILS_VERSION=2.30
export GCC_VERSION=8.1.0

export GNU_MIRROR_BASE=https://ftp.gnu.org/gnu
export ISL_MIRROR_BASE=http://isl.gforge.inria.fr
export CLOOG_MIRROR_BASE=https://www.bastoul.net/cloog/pages/download

export PATH=$HOME/SydOS-dev.framework:$PATH

function download_compile {
	echo $1
	curl $1 > $2.archive
	tar -xf $2.archive
	mkdir $2-build
	cd $2-build
	../$2/configure --prefix=$HOME/SydOS-dev.framework $3
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
download_compile $GNU_MIRROR_BASE/gmp/gmp-$GMP_VERSION.tar.bz2 gmp-$GMP_VERSION
download_compile $GNU_MIRROR_BASE/mpfr/mpfr-$MPFR_VERSION.tar.bz2 mpfr-$MPFR_VERSION "--with-gmp=$HOME/SydOS-dev.framework"
download_compile $GNU_MIRROR_BASE/mpc/mpc-$MPC_VERSION.tar.gz mpc-$MPC_VERSION "--with-gmp=$HOME/SydOS-dev.framework --with-mpfr=$HOME/SydOS-dev.framework"
download_compile $ISL_MIRROR_BASE/isl-$ISL_VERSION.tar.gz isl-$ISL_VERSION "--with-gmp-prefix=$HOME/SydOS-dev.framework"
download_compile $CLOOG_MIRROR_BASE/cloog-$CLOOG_VERSION.tar.gz cloog-$CLOOG_VERSION "--with-gmp-prefix=$HOME/SydOS-dev.framework --with-isl-prefix=$HOME/SydOS-dev.framework"
download_compile $GNU_MIRROR_BASE/binutils/binutils-$BINUTILS_VERSION.tar.gz binutils-$BINUTILS_VERSION "--with-isl=$HOME/SydOS-dev.framework --with-cloog=$HOME/SydOS-dev.framework --with-sysroot --disable-nls --disable-werror --target=i686-elf"
download_compile $GNU_MIRROR_BASE/gcc/gcc-$GCC_VERSION/gcc-$GCC_VERSION.tar.gz gcc-$GCC_VERSION "--with-isl=$HOME/SydOS-dev.framework --with-cloog=$HOME/SydOS-dev.framework --with-gmp=$HOME/SydOS-dev.framework --with-mpfr=$HOME/SydOS-dev.framework --with-mpc=$HOME/SydOS-dev.framework --disable-nls --enable-languages=c,c++ --without-headers --target=i686-elf"