# bouffalo_isp

bouffalo_isp is an ISP tool that supports flashing firmware into BouffaloLab chips for MPUs running on the Linux platform.

## Support CHIPS

|    CHIP        |   STATUS    |
|:--------------:|:-----------:|
|BL602/BL604     |   √         |
|BL702/BL704/BL706 | √         |
|BL616/BL618     |   √         |
|BL808           |   -         |

## Build & Flash

- Modify `CROSS_COMPILE` with your linux platform gcc in CMakeLists.txt
- Modify boot and reset pin in `user_config.h`
- Execute `make` and you can get bouffalo_isp in build directory
- Try `chmod +x ./bouffalo_isp` to make it executable
- Try `./bouffalo_isp` to get help
- Notice that your firmware must be whole bin(combined with bootheader and firmware and other bins)
- For example

```
$ ./bouffalo_isp
bouffalo isp for linux
Usage: bouffalo_isp [tty node] [baud rate] [download file path]
       Sample: bouffalo_isp /dev/ttyACM0 2000000 ./helloworld.bin

```