export GMP_VERSION=6.1.2
export MPFR_VERSION=4.0.1
export MPC_VERSION=1.1.0
# ISL 0.20 is incompatible with GCC 8.2.0, but will be fixed with GCC 8.3.0
export ISL_VERSION=0.19
export BINUTILS_VERSION=2.31.1
export GCC_VERSION=8.2.0
export GDB_VERSION=8.1.1
export CLOOG_VERSION=0.18.4
export NASM_VERSION=2.13.03
export PKG_CONFIG_VERSION=0.29.2
export LIBFFI_VERSION=3.2.1
export GETTEXT_VERSION=0.19.8.1
export PCRE_VERSION=8.42
export GLIB_VERSION=2.57.1
export PIXMAN_VERSION=0.34.0
export QEMU_VERSION=2.12.1
export GRUB_VERSION=2.02

export GNU_MIRROR_BASE=https://ftp.gnu.org/gnu
export ISL_MIRROR_BASE=http://isl.gforge.inria.fr
export CLOOG_MIRROR_BASE=https://www.bastoul.net/cloog/pages/download
export PKG_CONFIG_MIRROR_BASE=https://pkg-config.freedesktop.org/releases
export LIBFFI_MIRROR_BASE=http://sourceware.org/pub/libffi
export PCRE_MIRROR_BASE=https://ftp.pcre.org/pub/pcre
export GLIB_MIRROR_BASE=http://mirror.umd.edu/gnome/sources/glib/2.57
export CAIROGRAPHICS_MIRROR_BASE=https://www.cairographics.org/releases
export QEMU_MIRROR_BASE=https://download.qemu.org

export INSTALL_DIR=$HOME/SydOS-dev.framework
export PATH=$INSTALL_DIR/bin:$PATH

function download_compile {
	echo $1
	rm -rf $2*

	curl $1 -L > $2.archive
	tar -xf $2.archive
	
	mkdir $2-build
	cd $2-build

	if [ "$2" == "glib-$GLIB_VERSION" ]; then
		CFLAGS="-L$INSTALL_DIR/lib -I$INSTALL_DIR/include" ../$2/configure --prefix=$INSTALL_DIR $3
	else
		../$2/configure --prefix=$INSTALL_DIR $3
	fi

	if [ "$2" == "gcc-$GCC_VERSION" ]; then
		make all-gcc 
		make all-target-libgcc 
		make install-gcc
		make install-target-libgcc
	else
		if [ "$2" == "pixman-$PIXMAN_VERSION" ]; then
			cp ../utils-prng.c ../$2/test/utils-prng.c
		fi
		make 
		make install
	fi
	cd ..
	rm -rf $2*
}

set -e
