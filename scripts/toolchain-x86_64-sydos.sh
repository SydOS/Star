echo -e "\033[91m"
echo "__          __     _____  _   _ _____ _   _  _____ "
echo "\ \        / /\   |  __ \| \ | |_   _| \ | |/ ____|"
echo " \ \  /\  / /  \  | |__) |  \| | | | |  \| | |  __ "
echo "  \ \/  \/ / /\ \ |  _  /| . \` | | | | . \` | | |_ |"
echo "   \  /\  / ____ \| | \ \| |\  |_| |_| |\  | |__| |"
echo "    \/  \/_/    \_\_|  \_\_| \_|_____|_| \_|\_____|"
echo
echo "This toolchain is HIGHLY EXPERIEMENTAL."
echo "Using this toolchain has an increased likelyness of stuff breaking, crashing, imploding, exploiding, or destroying the universe."
echo "USE AT YOUR OWN RISK."
echo -e "\033[39m"

sleep 2s

source global.sh
export PATH=$HOME/SydOS-toolchain.framework/bin:$PATH

rm -rf autoconf-2.64 autoconf-2.64-build
tar -xf autoconf-2.64.tar.gz
mkdir autoconf-2.64-build
cd autoconf-2.64-build
../autoconf-2.64/configure --prefix=$HOME/SydOS-toolchain.framework
make
make install
cd ..

rm -rf automake-1.11.6 automake-1.11.6-build
tar -xf automake-1.11.6.tar.gz
mkdir automake-1.11.6-build
cd automake-1.11.6-build
../automake-1.11.6/configure --prefix=$HOME/SydOS-toolchain.framework
make
make install
cd ..

rm -rf gmp-6.1.2 gmp-6.1.2-build
tar -xf gmp-6.1.2.tar.bz2
mkdir gmp-6.1.2-build
cd gmp-6.1.2-build
../gmp-6.1.2/configure --prefix=$HOME/SydOS-toolchain.framework
make
make install
cd ..

rm -rf mpfr-4.0.1 mpfr-4.0.1-build
tar -xf mpfr-4.0.1.tar.gz
mkdir mpfr-4.0.1-build
cd mpfr-4.0.1-build
../mpfr-4.0.1/configure --prefix=$HOME/SydOS-toolchain.framework --with-gmp=$HOME/SydOS-toolchain.framework
make
make install
cd ..

rm -rf mpc-1.1.0 mpc-1.1.0-build
tar -xf mpc-1.1.0.tar.gz
mkdir mpc-1.1.0-build
cd mpc-1.1.0-build
../mpc-1.1.0/configure --prefix=$HOME/SydOS-toolchain.framework --with-gmp=$HOME/SydOS-toolchain.framework --with-mpfr=$HOME/SydOS-toolchain.framework
make
make install
cd ..

rm -rf isl-0.19 isl-0.19-build
tar -xf isl-0.19.tar.gz
mkdir isl-0.19-build
cd isl-0.19-build
../isl-0.19/configure --prefix=$HOME/SydOS-toolchain.framework --with-gmp-prefix=$HOME/SydOS-toolchain.framework
make
make install
cd ..

rm -rf cloog-0.18.4 cloog-0.18.4-build
tar -xf cloog-0.18.4.tar.gz
mkdir cloog-0.18.4-build
cd cloog-0.18.4-build
../cloog-0.18.4/configure --prefix=$HOME/SydOS-toolchain.framework --with-isl-prefix=$HOME/SydOS-toolchain.framework --with-gmp-prefix=$HOME/SydOS-toolchain.framework
make
make install
cd ..

rm -rf binutils-2.30 binutils-2.30-build
tar -xf binutils-2.30.tar.gz
patch -s -p0 < sydos-binutils.patch
cd binutils-2.30/ld
automake
cd ../..
mkdir binutils-2.30-build
cd binutils-2.30-build
../binutils-2.30/configure --prefix=$HOME/SydOS-toolchain.framework --with-isl=$HOME/SydOS-toolchain.framework --with-cloog=$HOME/SydOS-toolchain.framework --with-sysroot --disable-nls --disable-werror --target=x86_64-sydos
make
make install
make distclean
../binutils-2.30/configure --prefix=$HOME/SydOS-toolchain.framework --with-isl=$HOME/SydOS-toolchain.framework --with-cloog=$HOME/SydOS-toolchain.framework --with-sysroot --disable-nls --disable-werror --target=i386-sydos
make
make install
make distclean
../binutils-2.30/configure --prefix=$HOME/SydOS-toolchain.framework --with-isl=$HOME/SydOS-toolchain.framework --with-cloog=$HOME/SydOS-toolchain.framework --with-sysroot --disable-nls --disable-werror --target=i486-sydos
make
make install
make distclean
../binutils-2.30/configure --prefix=$HOME/SydOS-toolchain.framework --with-isl=$HOME/SydOS-toolchain.framework --with-cloog=$HOME/SydOS-toolchain.framework --with-sysroot --disable-nls --disable-werror --target=i586-sydos
make
make install
make distclean
../binutils-2.30/configure --prefix=$HOME/SydOS-toolchain.framework --with-isl=$HOME/SydOS-toolchain.framework --with-cloog=$HOME/SydOS-toolchain.framework --with-sysroot --disable-nls --disable-werror --target=i686-sydos
make
make install
make distclean
../binutils-2.30/configure --prefix=$HOME/SydOS-toolchain.framework --with-isl=$HOME/SydOS-toolchain.framework --with-cloog=$HOME/SydOS-toolchain.framework --with-sysroot --disable-nls --disable-werror --target=i786-sydos
make
make install
cd ..

#rm -rf binutils-$BINUTILS_VERSION*
#curl $GNU_MIRROR_BASE/binutils/binutils-$BINUTILS_VERSION.tar.gz > binutils-$BINUTILS_VERSION.archive
#tar -xf binutils-$BINUTILS_VERSION.archive
#patch -s -p0 < SydOS-binutils.patch
#mkdir binutils-$BINUTILS_VERSION-build
#cd binutils-$BINUTILS_VERSION-build

#../binutils-$BINUTILS_VERSION/configure --prefix=$INSTALL_DIR --with-isl=$INSTALL_DIR --with-cloog=$INSTALL_DIR --with-sysroot --disable-nls --disable-werror --target=x86_64-elf
#make
#make install

#cd ..
#rm -rf binutils-$BINUTILS_VERSION*