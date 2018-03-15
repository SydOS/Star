set -e
# i686 ALWAYS comes first
./toolchain-i686.sh
./toolchain-i386.sh
./toolchain-i486.sh
./toolchain-i586.sh
./toolchain-x86_64.sh
./toolchain-nasm.sh