# NuPhy Air75 V2

![NuPhy Air75 V2](https://bit.ly/486pSkN)

A 75% wireless mechanical keyboard with 84 keys, supporting USB, 2.4GHz RF, and Bluetooth connectivity.

* Keyboard Maintainer: [nuphy](https://github.com/nuphy-src)
* Hardware Supported: NuPhy Air75 V2 (ANSI layout)
* Hardware Availability: [nuphy.com](https://nuphy.com)
* MCU: STM32F072

Make example for this keyboard (after setting up your build environment):

    make nuphy/air75_v2/ansi:default

Flashing example for this keyboard:

    make nuphy/air75_v2/ansi:default:flash

See the [build environment setup](https://docs.qmk.fm/#/getting_started_build_tools) and the [make instructions](https://docs.qmk.fm/#/getting_started_make_guide) for more information. Brand new to QMK? Start with our [Complete Newbs Guide](https://docs.qmk.fm/#/newbs).

## Bootloader

Enter the bootloader in one way:

* **Bootmagic reset**: Hold down the key at (0,0) in the matrix (Escape) and plug in the keyboard
