# GalioEthernet9601

![Galio.jpg](./Documentation/Galio.jpg)

[English](https://github.com/startpenghubingzhou/GalioEthernet9601/blob/main/README.md)|[简体中文](https://github.com/startpenghubingzhou/GalioEthernet9601/blob/main/README-CN.md)

我做了这个项目来表达我对加里奥的致敬，他是我在英雄联盟里最喜欢的英雄之一。（皮这一下我很开心）

好了，让我们回归正题吧。 GalioEthernet9601是一个基于Davicom公司的DM9601 USB转接器开发的MacOS开源驱动  (详细说明请见 [samuelv0304的项目]( https://github.com/samuelv0304/USBCDCEthernet)).。实际上，你可以把它看作是对**USBCDCEthernet**的一个重写和升级的项目。

## 为什么我要重写USBCDCEthernet?

要承认的是， USBCDCEthernet是一个十分完善的驱动,但它仍然有很多问题。比如，它项目里的代码是给10.11及以前版本的，以至于在最新版MacOS上你没法编译它。除此之外，USBCDCEthernet项目已经7年没有维护了。当我想尝试在其中添加一些新功能时，我发现我做不到，因为我不得不去瞎子旧版本的MacOS，这对我来说显然是不现实的。

所以就这么简单，我重写了这个驱动。

## 它能做什么?

就像我说的一样，这是一个由USBCDCEthernet **重写的** 项目，因此理论上 USBCDCEthernet能做到什么，它也能做到什么。目前为止我还没有计划加入一些新的东西，因为在IOKit方面我目前还是个新手。 也许有一天，当它完全移植了USBCDCEthernet的东西后，我就可以着手加些新东西了。但至少目前为止，这个驱动能够在你的MacOS/黑苹果上做到这些：

- 检测你的转接器(并不能真正工作，相关支持即将上线)
- 对你的转接器的基本支持

如果你对这个项目有一些好主意，只需要在issue页面写下来就行。如果你有好的代码要贡献，欢迎随时送达PR给我。

## 我怎样安装它?

- 从这里下载并用XCode编译它。

- 使用sudo把驱动拷贝到/System/Library/Extensions然后重建缓存(或者通过OC/Clover注入，仅限黑苹果)。

我在13.6上已经测试过，它很可能支持12.0以上版本，但我目前尚未测试。

## 文档

从 [这里](https://github.com/startpenghubingzhou/GalioEthernet9601/blob/main/Documentation/DM9601-DS-P01-930914.pdf) 获得更多信息。



## 鸣谢

- @[haiku](https://github.com/haiku) 的 [haiku project](https://github.com/haiku/haiku)
- @[samuelv0304](https://github.com/samuelv0304) 的 [USBCDCEthernet](https://github.com/samuelv0304/USBCDCEthernet)**(致以最崇高的敬意！)**
- @[zxystd ](https://github.com/zxystd) 对此项目编写的帮助

## 许可证

这个项目的代码遵循 **苹果公共源代码许可证（ASPL）**。点击License可以查询更多相关信息。

