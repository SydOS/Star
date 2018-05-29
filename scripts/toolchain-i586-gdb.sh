export PATH=$HOME/SydOS-dev.framework:$PATH

function download_compile {
	echo $1
	curl $1 > $2.archive
	tar -xf $2.archive
	mkdir $2-build
	cd $2-build
	../$2/configure --prefix=$HOME/SydOS-dev.framework $3
	make -j$(getconf _NPROCESSORS_ONLN)
	make install
	cd ..
	rm -rf $2*
}

set -e
download_compile https://ftp.gnu.org/gnu/gdb/gdb-8.1.tar.gz gdb-8.1 "--target=i586-elf"