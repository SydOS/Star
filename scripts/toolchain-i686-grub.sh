source global.sh

download_compile $GNU_MIRROR_BASE/grub/grub-$GRUB_VERSION.tar.gz grub-$GRUB_VERSION "--disable-werror TARGET_CC=i686-elf-gcc TARGET_OBJCOPY=i686-elf-objcopy TARGET_STRIP=i686-elf-strip TARGET_NM=i686-elf-nm TARGET_RANLIB=i686-elf-ranlib --target=i686-elf"