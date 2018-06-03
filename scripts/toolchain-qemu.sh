#export PKG_CONFIG_PATH=$HOME/test/lib/pkgconfig
#
#pkg-config ./configure --prefix=$HOME/test --with-internal-glib
#
#http://sourceware.org/pub/libffi/libffi-3.2.1.tar.gz
#libffi
#
#http://ftp.gnu.org/pub/gnu/gettext/
#gettext
#
#https://ftp.pcre.org/pub/pcre/
#pcre --enable-unicode-properties
#
#https://download.gnome.org/sources/glib/2.56/
#glib CFLAGS="-L/Users/sydneyerickson/test/lib -I/Users/sydneyerickson/test/include" ./configure --prefix=/Users/sydneyerickson/test/
#
#https://www.cairographics.org/releases/
#pixman
#
#qemu --target-list=x86_64-softmmu


source global.sh

export PKG_CONFIG_PATH=$INSTALL_DIR/lib/pkgconfig

download_compile $PKG_CONFIG_MIRROR_BASE/pkg-config-$PKG_CONFIG_VERSION.tar.gz pkg-config-$PKG_CONFIG_VERSION "--with-internal-glib"
download_compile $LIBFFI_MIRROR_BASE/libffi-$LIBFFI_VERSION.tar.gz libffi-$LIBFFI_VERSION
download_compile $GNU_MIRROR_BASE/gettext/gettext-$GETTEXT_VERSION.tar.gz gettext-$GETTEXT_VERSION
download_compile $PCRE_MIRROR_BASE/pcre-$PCRE_VERSION.tar.gz pcre-$PCRE_VERSION "--enable-unicode-properties"
download_compile $GLIB_MIRROR_BASE/glib-$GLIB_VERSION.tar.xz glib-$GLIB_VERSION
download_compile $CAIROGRAPHICS_MIRROR_BASE/pixman-$PIXMAN_VERSION.tar.gz pixman-$PIXMAN_VERSION
download_compile $QEMU_MIRROR_BASE/qemu-$QEMU_VERSION.tar.bz2 qemu-$QEMU_VERSION "--target-list=x86_64-softmmu"