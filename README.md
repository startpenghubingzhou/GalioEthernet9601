# GalioEthernet9601

![Galio.jpg](./Documentation/Galio.jpg)



I wrote this project to pay my respect to Galio, which is one of my favorite champions in LOL.:）

OK let's get to the point. GalioEthernet9601 is an open source driver for the USB-Ethernet adapter based on Davicom DM9601 for MacOS/Hackinstosh (See [samuelv0304' s project]( https://github.com/samuelv0304/USBCDCEthernet) for more details). Actually, you could regard this project as a **Re-writed** and **Upgraded** driver compared with **USBCDCEthernet**.

## Why I rewrote USBCDCEthernet?

Though complete USBCDCEthernet is, it still have some issues. For example, the code in this project is for 10.11 and ealier so that you can't compile it in the latest version of MacOS. What' s more,  USBCDCEthernet has not been maintained for about 7 years. When I try to add some new features on it, I found I can't do it as I need to download an old version of MacOS, which is impossible for me. 

So that' s it , I wrote this new project.

## What can it do?

As I said, this is a **Re-writed** project from USBCDCEthernet, so it can do what USBCDCEthernet can do in theory. For now I have no plans to add some new features on it because  I  ' m really a Green-hand on IOKit. Maybe I could get down to add some new features on it as soon as it is completely transplanted from USBCDCEthernet. But at least, this driver can do these things on your MacOS/Hackintosh:

- Detect your Adapter(not really work, will work soon)
- Bascially support for your Adapter

If you have some ideas about this project, just write it in issue page. If you have some new code, just send PR to me.

## How Could I install it?

- Download the source here from Github and compile it with XCode

- Copy this driver  in /System/Library/Extensions with 'sudo' command and then rebuild the kextcache(Or inject it into OC/Clover, only for Hackintosh).

I have tested in 13.6 and it may support in 12.0+, but haven' t tested yet.

## License

See [here](https://github.com/startpenghubingzhou/GalioEthernet9601/blob/main/Documentation/DM9601-DS-P01-930914.pdf) for more information.



##Credits

- @[haiku](https://github.com/haiku) for [haiku project](https://github.com/haiku/haiku)
- @[samuelv0304](https://github.com/samuelv0304) for [USBCDCEthernet](https://github.com/samuelv0304/USBCDCEthernet)**(THE HIGHEST RESPECT)**
- @[zxystd ](https://github.com/zxystd) for help in writing

## License

This code is licensed under **Apple Public Source License**. See LICENSE for more details.

