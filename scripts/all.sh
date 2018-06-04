set -e
# i686 ALWAYS comes first
./toolchain-i686.sh
./toolchain-i686-gdb.sh
./toolchain-i386.sh
./toolchain-i386-gdb.sh
./toolchain-i486.sh
./toolchain-i486-gdb.sh
./toolchain-i586.sh
./toolchain-i586-gdb.sh
./toolchain-x86_64.sh
./toolchain-x86_64-gdb.sh
./toolchain-i686-grub.sh
./toolchain-nasm.sh
./toolchain-qemu.sh