export GMP_VERSION=6.1.2
export MPFR_VERSION=3.1.6
export MPC_VERSION=1.0.3
export ISL_VERSION=0.18
export CLOOG_VERSION=0.18.4
export BINUTILS_VERSION=2.29.1
export GCC_VERSION=7.2.0

export GNU_MIRROR_BASE=https://ftp.gnu.org/gnu
export ISL_MIRROR_BASE=http://isl.gforge.inria.fr
export CLOOG_MIRROR_BASE=https://www.bastoul.net/cloog/pages/download/count.php3?url=./

export PATH=$HOME/tools:$PATH

function download_compile {
	echo $1
	curl $1 > $2.archive
	tar -xf $2.archive
	cd $2
	./configure --prefix=$HOME/tools $3
	make
	make install
	cd ..
	rm -rf $2.archive
	rm -rf $2
}

set -e
download_compile $GNU_MIRROR_BASE/gmp/gmp-$GMP_VERSION.tar.bz2 gmp-$GMP_VERSION
download_compile $GNU_MIRROR_BASE/mpfr/mpfr-$MPFR_VERSION.tar.bz2 mpfr-$MPFR_VERSION "--with-gmp=$HOME/tools"
download_compile $GNU_MIRROR_BASE/mpc/mpc-$MPC_VERSION.tar.gz mpc-$MPC_VERSION "--with-gmp=$HOME/tools --with-mpfr=$HOME/tools"
download_compile $ISL_MIRROR_BASE/isl-$ISL_VERSION.tar.gz isl-$ISL_VERSION "--with-gmp-prefix=$HOME/tools"
download_compile $CLOOG_MIRROR_BASE/cloog-$CLOOG_VERSION.tar.gz cloog-$CLOOG_VERSION "--with-gmp-prefix=$HOME/tools --with-isl-prefix=$HOME/tools"
download_compile $GNU_MIRROR_BASE/binutils/binutils-$BINUTILS_VERSION.tar.gz binutils-$BINUTILS_VERSION "--with-isl=$HOME/tools --with-cloog=$HOME/tools --with-sysroot --disable-nls --disable-werror --target=x86_64-elf"
download_compile $GNU_MIRROR_BASE/gcc/gcc-$GCC_VERSION/gcc-$GCC_VERSION.tar.gz gcc-$GCC_VERSION "--with-isl=$HOME/tools --with-cloog=$HOME/tools --with-gmp=$HOME/tools --with-mpfr=$HOME/tools --with-mpc=$HOME/tools --disable-nls --enable-languages=c,c++ --without-headers --target=x86_64-elf"