source global.sh

download_compile $GNU_MIRROR_BASE/gmp/gmp-$GMP_VERSION.tar.bz2 gmp-$GMP_VERSION
download_compile $GNU_MIRROR_BASE/mpfr/mpfr-$MPFR_VERSION.tar.bz2 mpfr-$MPFR_VERSION "--with-gmp=$INSTALL_DIR"
download_compile $GNU_MIRROR_BASE/mpc/mpc-$MPC_VERSION.tar.gz mpc-$MPC_VERSION "--with-gmp=$INSTALL_DIR --with-mpfr=$INSTALL_DIR"
download_compile $ISL_MIRROR_BASE/isl-$ISL_VERSION.tar.gz isl-$ISL_VERSION "--with-gmp-prefix=$INSTALL_DIR"
download_compile $CLOOG_MIRROR_BASE/cloog-$CLOOG_VERSION.tar.gz cloog-$CLOOG_VERSION "--with-gmp-prefix=$INSTALL_DIR --with-isl-prefix=$INSTALL_DIR"
download_compile $GNU_MIRROR_BASE/binutils/binutils-$BINUTILS_VERSION.tar.gz binutils-$BINUTILS_VERSION "--with-isl=$INSTALL_DIR --with-cloog=$INSTALL_DIR --with-sysroot --disable-nls --disable-werror --target=i686-elf"
download_compile $GNU_MIRROR_BASE/gcc/gcc-$GCC_VERSION/gcc-$GCC_VERSION.tar.gz gcc-$GCC_VERSION "--with-isl=$INSTALL_DIR --with-cloog=$INSTALL_DIR --with-gmp=$INSTALL_DIR --with-mpfr=$INSTALL_DIR --with-mpc=$INSTALL_DIR --disable-nls --enable-languages=c,c++ --without-headers --target=i686-elf"
