Little OS
=========

[Littel OS Book][littleosbook] is a practical guide to writing
your own x86 operating system. It is designed to give enough help
with the technical details while at the same time not reveal
too much with samples and code excerpts.

It foucs on writing kernel and drivers. It just uses grub as
the bootloader.

Our repo will complete the code in this book.

structure
---------

[loader.s](./loader.s) is the entrance of the kernel

drivers
-------

A driver acts as a layer between kernel and hardware,
providing a higher abstraction than communicating directly
with the hardware. For example, output driver
hides the detail hardware operation and provide
a `write` function with the following declaration:

```bash
int write(char *buf, unsigned int len);
```

The kernel can use it directly to write a buffer to the console.
Detailed hareware operations are implemented in
[io.s](./io.s) and [framebuffer.c](./framebuffer.c).


[littleosbook]: https://littleosbook.github.io/