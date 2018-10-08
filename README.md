# SydOS

Welcome to SydOS! Me, along with a couple of friends (and hopefully you!) are working on a small toy kernel. Someday it
might be suitable for some form of use. I'm targetting getting it to run inside virtual machines as the guest, but you're
welcome to play around and make it work on real machines if you want. I'll *probably* PR them.

## Getting Started

These instructions will get you a copy of the project up and running on your local machine for development and testing purposes.

### Prerequisites

You will need the NASM compiler, QEMU, and an i686-elf toolchain. You can compile the others if you wanted to, but it's not needed.
**NOTE:** the toolchain build script will place all binaries in `$HOME/tools`. You must add this to your PATH variable.

```
brew install nasm qemu
cd scripts && ./toolchain-i686.sh
```

### Building

You can build the kernel just like this.

```
ARCH=i686 make build-kernel
```

### Testing

You can do two types of "tests". A quick one, and a GDB one. The following will just boot QEMU, pointed to the kernel.
If anything goes wrong, there's no way to really debug it, but it's decent to see if your code completely breaks everything.
Serial will be pointed to `stdio` on your console. If you're using a text editor or IDE this probably won't work.

```
make test
```

A more indepth GDB based testing session can be achieved like below. No fancy serial redirection exists here, since the
console will be assigned to GDB. Serial will be inside QEMU's view options.

```
make debug
```

## Versioning

We use [SemVer](http://semver.org/) for versioning. For the versions available, see the [tags on this repository](https://github.com/1byte2bytes/SydOS/releases). 

## Authors

* **Sydney Erickson** - *Project Owner and Manager* - [1byte2bytes](https://github.com/1byte2bytes)
* **John Davis** - *Computer Wizard, driver writer and debugger* - [Goldfish64](https://github.com/Goldfish64)

See also the list of [contributors](https://github.com/1byte2bytes/SydOS/graphs/contributors) who participated in this project.

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

## Acknowledgments

Thanks to the [osdev.org](https://wiki.osdev.org/Main_Page) wiki, [osdever.net](http://www.osdever.net/), James Molloy's [kernel tutorials](http://www.jamesmolloy.co.uk/tutorial_html/), BrokenThorn Entertainment's [OS dev tutorials](http://www.brokenthorn.com/Resources/OSDevIndex.html), and everyone else online who's put up resources or asked questions we've had before. Keep being curious, it helps us all!
