# VMDK Dumper

This is a tool designed to extract data from an incomplete VMDK on a best-effort basis. It is only compatible with the Hosted Sparse Extents variant of the format.

Please note that incompleteness is the only use case. Any sort of other corruption is not worked around and will cause the tool to exit with an error.
Of course you can also use it on a normal complete sparse vmdk in order to transform it into a raw image, but there are other more established tools such as `qemu-img` that would be a better choice for such a scenario.

## Usage

* Process file without decompressing/writing anything: `./vmdk_dump input.vmdk`
* Write output: `./vmdk_dump input.vmdk dump.raw`

## Compiling

Simply run `make`. The only development requirements are a compiler accepting GCC syntax (`packed` attribute is used in a header) and zlib headers.

## Format notes

https://github.com/lurker0/PEkernel/blob/master/Documents/vmdk_50_technote.pdf
https://github.com/libyal/libvmdk/blob/main/documentation/VMWare%20Virtual%20Disk%20Format%20(VMDK).asciidoc
