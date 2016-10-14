# Dreamcast controller to USB adapter firmware

AVR micro-controller firmware for a Dreamcast controller to USB adapter.

## Project homepage

Schematic and additional information are available on the project homepage:

English: [Dreamcast controller to USB adapter](http://www.raphnet.net/electronique/dreamcast_usb/index_en.php)
French: [Adaptateur manette Dreamcast à USB](http://www.raphnet.net/electronique/dreamcast_usb/index.php)

## Supported micro-controllers

Currently supported micro-controllers:

* Atmega168

Adding support for other micro-controllers should be easy, as long as the target has enough
IO pins, enough memory (flash and SRAM) and is supported by V-USB.

## Built with

* [avr-gcc](https://gcc.gnu.org/wiki/avr-gcc)
* [avr-libc](http://www.nongnu.org/avr-libc/)
* [gnu make](https://www.gnu.org/software/make/manual/make.html)

## License

This project is licensed under the terms of the GNU General Public License, version 2.

## Acknowledgments

* Thank you to Objective development, author of [V-USB](https://www.obdev.at/products/vusb/index.html) for a wonderful software-only USB device implementation.
