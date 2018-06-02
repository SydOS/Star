source global.sh

download_compile $GNU_MIRROR_BASE/binutils/binutils-$BINUTILS_VERSION.tar.gz binutils-$BINUTILS_VERSION "--with-isl=$INSTALL_DIR --with-cloog=$INSTALL_DIR --with-sysroot --disable-nls --disable-werror --target=i486-elf"
download_compile $GNU_MIRROR_BASE/gcc/gcc-$GCC_VERSION/gcc-$GCC_VERSION.tar.gz gcc-$GCC_VERSION "--with-isl=$INSTALL_DIR --with-cloog=$INSTALL_DIR --with-gmp=$INSTALL_DIR --with-mpfr=$INSTALL_DIR --with-mpc=$INSTALL_DIR --disable-nls --enable-languages=c,c++ --without-headers --target=i486-elf"
