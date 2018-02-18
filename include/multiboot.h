// Multiboot stuff:
// https://www.gnu.org/software/grub/manual/multiboot/multiboot.html
// https://anastas.io/osdev/memory/2016/08/08/page-frame-allocator.html

#ifndef MULTIBOOT_HEADER_MAGIC
#define MULTIBOOT_HEADER

// How many bytes from the start of the file we search for the header.
#define MULTIBOOT_SEARCH                8192
#define MULTIBOOT_HEADER_ALIGN          4

// Magic value, the magic field must contain this.
#define MULTIBOOT_HEADER_MAGIC          0x1BADB002

// Bootloader magic value, should be in EAX.
#define MULTIBOOT_BOOTLOADER_MAGIC      0x2BADB002

// Alignment of Multiboot modules.
#define MULTIBOOT_MOD_ALIGN             0x00001000

// Alignment of Multiboot info structure.
#define MULTIBOOT_INFO_ALIGN            0x00000004

//
// Flags set in the flags member of the header.
//
// Align all boot modules on 4KB boundaries.
#define MULTIBOOT_PAGE_ALIGN            0x00000001
// Must pass memory info to OS.
#define MULTIBOOT_MEMORY_INFO           0x00000002
// Must pass video info to OS.
#define MULTIBOOT_VIDEO_MODE            0x00000004
// Indicates the use of address fields in header.
#define MULTIBOOT_AOUT_KLUDGE           0x00010000

//
// Flags set in the Multiboot info structure.
//
// Is there basic lower/upper memory information?
#define MULTIBOOT_INFO_MEMORY           0x00000001
// Is there a boot device set?
#define MULTIBOOT_INFO_BOOTDEV          0x00000002
// Is the command-line defined?
#define MULTIBOOT_INFO_CMDLINE          0x00000004
// Are there modules to do something with?
#define MULTIBOOT_INFO_MODS             0x00000008
// Is there a symbol table loaded? Mutally exclusive with MULTIBOOT_INFO_ELF_SHDR.
#define MULTIBOOT_INFO_AOUT_SYMS        0x00000010
// Is there an ELF section header table? Mutally exclusive with MULTIBOOT_INFO_AOUT_SYMS.
#define MULTIBOOT_INFO_ELF_SHDR         0x00000020
// Is there a full memory map?
#define MULTIBOOT_INFO_MEM_MAP          0x00000040
// Is there drive info?
#define MULTIBOOT_INFO_DRIVE_INFO       0x00000080
// Is there a config table?
#define MULTIBOOT_INFO_CONFIG_TABLE     0x00000100
// Is there a boot loader name?
#define MULTIBOOT_INFO_BOOT_LOADER_NAME 0x00000200
// Is there an APM table?
#define MULTIBOOT_INFO_APM_TABLE        0x00000400

//
// Multiboot framebuffer types.
//
#define MULTIBOOT_FRAMEBUFFER_TYPE_INDEXED  0
#define MULTIBOOT_FRAMEBUFFER_TYPE_RGB      1
#define MULTIBOOT_FRAMEBUFFER_TYPE_EGA_TEXT 2

//
// Multiboot memory entry types.
#define MULTIBOOT_MEMORY_AVAILABLE          1
#define MULTIBOOT_MEMORY_RESERVED           2
#define MULTIBOOT_MEMORY_ACPI_RECLAIMABLE   3
#define MULTIBOOT_MEMORY_NVS                4
#define MULTIBOOT_MEMORY_BADRAM             5 

// Is there video information?
#define MULTIBOOT_INFO_VBE_INFO         0x00000800
#define MULTIBOOT_INFO_FRAMEBUFFER_INFO 0x00001000

// Multiboot header.
struct multiboot_header
{
    // Magic number.
    uint32_t magic;

    // Feature flags.
    uint32_t flags;

    // Checksum. The two above fields plus this one must equal 0 % 2^32.
    uint32_t checksum;

    // Kludge info (valid only if MULTIBOOT AOUT KLUDGE is set).
    uint32_t header_addr;
    uint32_t load_addr;
    uint32_t load_end_addr;
    uint32_t bss_end_addr;
    uint32_t entry_addr;

    // Video mode info (valid only if MULTIBOOT VIDEO MODE is set).
    uint32_t mode_type;
    uint32_t width;
    uint32_t height;
    uint32_t depth;
} __attribute__((packed));
typedef struct multiboot_header multiboot_header_t;

// Multiboot a.out symbol table.
struct multiboot_aout_symbol_table
{
    uint32_t tabsize;
    uint32_t strsize;
    uint32_t addr;
    uint32_t reserved;
} __attribute__((packed));
typedef struct multiboot_aout_symbol_table multiboot_aout_symbol_table_t;

// Multiboot ELF header table.
struct multiboot_elf_section_header_table
{
    uint32_t num;
    uint32_t size;
    uint32_t addr;
    uint32_t shndx;
} __attribute__((packed));
typedef struct multiboot_elf_section_header_table multiboot_elf_section_header_table_t;

// Multiboot info structure.
struct multiboot_info
{
    // Version number.
    uint32_t flags;

    // Available memory as reported by the BIOS.
    uint32_t mem_lower;
    uint32_t mem_upper;

    // "Root" partition.
    uint32_t boot_device;

    // Kernel command line.
    uint32_t cmdline;

    // Boot module list.
    uint32_t mods_count;
    uint32_t mods_addr;

    // a.out and ELF tables.
    union
    {
        multiboot_aout_symbol_table_t aout_sym;
        multiboot_elf_section_header_table_t elf_sec;
    } u;

    // Memory mapping.
    uint32_t mmap_length;
    uint32_t mmap_addr;

    // Drive info.
    uint32_t drives_length;
    uint32_t drives_addr;

    // ROM config table.
    uint32_t config_table;

    // Boot loader name.
    uint32_t boot_loader_name;

    // APM table.
    uint32_t apm_table;

    // Video.
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
    
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t framebuffer_bpp;
    uint8_t framebuffer_type;

    union
    {
        struct
        {
            uint32_t framebuffer_palette_addr;
            uint16_t framebuffer_palette_num_colors;
        } __attribute__((packed));
        struct
        {
            uint8_t framebuffer_red_field_position;
            uint8_t framebuffer_red_mask_size;
            uint8_t framebuffer_green_field_position;
            uint8_t framebuffer_green_mask_size;
            uint8_t framebuffer_blue_field_position;
            uint8_t framebuffer_blue_mask_size;
        } __attribute__((packed));
    };
} __attribute__((packed));
typedef struct multiboot_info multiboot_info_t;

struct multiboot_color
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} __attribute__((packed));
typedef struct multiboot_color multiboot_color_t;

// Multiboot memory map entry.
struct multiboot_mmap_entry
{
    uint32_t size;
    uint32_t addr;
    uint32_t len;
    uint32_t type;
} __attribute__((packed));
typedef struct multiboot_mmap_entry multiboot_memory_map_t;

// Multiboot module list.
struct multiboot_mod_list
{
    // Memory used by modules from mod_start to mod_end - 1.
    uint32_t mod_start;
    uint32_t mod_end;

    // Command line.
    uint32_t cmdline;

    // Padding to make 16 bytes (this must be zero).
    uint32_t pad;
} __attribute__((packed));
typedef struct multiboot_mod_list multiboot_module_t;

// Multiboot APM info.
struct multiboot_apm_info
{
    uint16_t version;
    uint16_t cseg;
    uint32_t offset;
    uint16_t cseg_16;
    uint16_t dseg;
    uint16_t flags;
    uint16_t cseg_len;
    uint16_t cseg_16_len;
    uint16_t dseg_len;
} __attribute__((packed));
typedef struct multiboot_apm_info multiboot_apm_info_t;

#endif
