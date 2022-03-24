# Operating Systems Capstone 2022

| GitHub account name | Student ID | Name   |
| ------------------- | ---------- | ------ |
| cl3nn0              | 310551034  | 張逸軍大  |

## How to build

```bash
make
```

## How to Run on QEMU

```bash
make run
```

## How to Run on Rpi3

- put files in SD card

    - bootcode.bin

    - fixup.dat

    - start.elf

    - bcm2710-rpi-3-b-plus.dtb

    - bootloader.img

    - config.txt

    - initramfs.cpio

```bash
python3 send_kernel.py
screen /dev/cu.usbserial-0001 115200
```

