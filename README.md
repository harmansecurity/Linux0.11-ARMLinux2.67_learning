# Linux0.11 + ARM Linux2.6.37内核相关学习

　 　这些资料都是去年暑假在实习的时候做的一些总结，趁着还没完全遗忘，这两天就想把以前做的东西整理出来。内核跟现在的研究方向关系不是很大，也好久没去倒腾了，当初也只是学习一个皮毛，这里分享出来供大家学习，求大神路过轻喷。

　 　当时主要研究内容是针对嵌入式系统常见异常问题，以保障嵌入式设备系统安全为目标，提出一种基于内存行为的系统异常检测机制。**当时我们得到的一个方法是：通过比对不同时间内所访问的内核函数，检测系统是否异常。**那么该如何拿到在运行过程中访问了哪些内核函数呢？当时尝试了很多方法，最后的结论是不修改内核是无法拿到运行过程中访问了哪些内核函数。以下的几篇文章会介绍系统调用原理和如何获取系统调用号。

### 解决的大概思路：
　 　用户运行一个程序，该程序所创建的进程开始是运行在用户态的，如果要执行文件操作，网络数据发送等操作，必须通过write，send等系统调用，这些系统调用会调用内核中的代码来完成操作，这时，必须切换到Ring0，然后进入3GB-4GB中的内核地址空间去执行这些代码完成操作，完成后，切换回Ring3，回到用户态。

　 　因为用户程序不能直接调用系统调用函数，我们经常看到的比如 fork、open、write 等等函数实际上并不是真正的系统调用函数，他们都只是c库，在这些函数里将执行一个软中断 swi 指令。所以决定在进入系统调用函数入口地址前加一条指令，该指令主要是指向我自己写的函数，函数里面实现获取到系统调用号，不影响整个中断的执行。

**如果觉得不错，请先在这个仓库上点个 star 吧**，我会继续跟大家分享自己的学习过程。

不定时进行调整和补充，需要关注更新的请 Watch、Star、Fork

-----

## 目录

- 这里是整理出来的一些文章，包括当时所做的实验，步骤详细。
  - [(1)系统调用hook技术总结](http://harmansecurity.cn/2017/05/19/%E7%B3%BB%E7%BB%9F%E8%B0%83%E7%94%A8Hook%E6%8A%80%E6%9C%AF%E6%80%BB%E7%BB%93/)
  - [(2)Bochs运行Linux0.11内核](http://harmansecurity.cn/2017/05/19/Bochs%E8%BF%90%E8%A1%8CLinux0.11%E5%86%85%E6%A0%B8/)
  - [(3)Linux0.11获取系统调用号](http://harmansecurity.cn/2017/05/19/Linux0.11%E8%8E%B7%E5%8F%96%E7%B3%BB%E7%BB%9F%E8%B0%83%E7%94%A8%E5%8F%B7/)
  - [(4)Linux0.11内核添加系统调用](http://harmansecurity.cn/2017/05/19/Linux0.11%E5%86%85%E6%A0%B8%E6%B7%BB%E5%8A%A0%E7%B3%BB%E7%BB%9F%E8%B0%83%E7%94%A8/)
  - [(5)用户态和内核态](http://harmansecurity.cn/2017/05/19/%E7%94%A8%E6%88%B7%E6%80%81%E5%92%8C%E5%86%85%E6%A0%B8%E6%80%81/)
  - [(6)Linux0.11系统调用原理 ](http://harmansecurity.cn/2017/05/19/Linux0.11%E7%B3%BB%E7%BB%9F%E8%B0%83%E7%94%A8%E5%8E%9F%E7%90%86/)
  - [(7)QEMU安装和调试运行Linux0.11](http://harmansecurity.cn/2017/05/19/QEMU%E5%AE%89%E8%A3%85%E5%92%8C%E8%B0%83%E8%AF%95%E8%BF%90%E8%A1%8CLinux0.11/)
  - [(8)ARM Linux2.6.37系统调用原理 ](http://harmansecurity.cn/2017/05/19/ARM%20Linux2.6.37%E7%B3%BB%E7%BB%9F%E8%B0%83%E7%94%A8%E5%8E%9F%E7%90%86/)
  - [(9)ARM Linux2.6.37中获取系统调用号 ](http://harmansecurity.cn/2017/05/19/ARM%20Linux2.6.37%E4%B8%AD%E8%8E%B7%E5%8F%96%E7%B3%BB%E7%BB%9F%E8%B0%83%E7%94%A8%E5%8F%B7/)

-----

## 源码说明

- [hookdemo](/hookdemo):利用hook技术写的一个小demo
- [Linux0.11获取系统调用号](/Linux0.11获取系统调用号):Linux0.11获取系统调用号源码
- [hit-oslab-linux-20110823](/hit-oslab-linux-20110823/oslab):oslab,一个很好的Linux0.11实验平台
- [Linux-0.11-master](/Linux-0.11-master):可直接编译成功的Linux0.11源码
- [linux-0.11-devel-040809](/linux-0.11-devel-040809):集成Linux0.11的Bochs编译环境
- [ARMLinux2.6.37获取调用号](/ARMLinux2.6.37获取调用号):ARMLinux2.6.37获取调用号源码
- [**技术报告**](/):**核心报告**,对两个月的实习成果做了一个总结


-----

# 联系作者

- [Harman's Personal Website](http://harmansecurity.cn/)
- 邮箱：`lianghui_1994@163.com`

-----

# Lisence

Lisenced under [Apache 2.0 lisence](http://opensource.org/licenses/Apache-2.0)