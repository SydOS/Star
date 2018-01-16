#if defined(_WIN32) || defined(_WIN64) || \
	defined(__linux__) || defined(__linux) || defined(linux) || \
	defined(__unix__) || defined(__unix) || defined(unix) || \
	defined(__APPLE__) || defined(__MACH__) || \
	defined(__FreeBSD__)
#error "You don't appear to be using the proper cross-compiler toolchain! (Wrong target OS)"
#endif

#if !defined(__i386__)
#error "You don't appear to be using the proper cross-compiler toolchain! (Wrong architecture)"
#endif