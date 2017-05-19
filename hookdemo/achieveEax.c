#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/unistd.h>
#include <linux/sched.h>
#include <linux/syscalls.h>
MODULE_LICENSE("Dual BSD/GPL");

#ifndef mdelay
#define mdelay(n)    (/
(__builtin_constant_p(n) &amp;&amp; (n)&lt;=MAX_UDELAY_MS) ? udelay((n)*1000) : /
({unsigned long __ms=(n); while (__ms--) udelay(1000);}))
#endif

#ifndef ndelay
#define ndelay(x) udelay(((x)+999)/1000)
#endif

#define _DEBUG
#ifdef _DEBUG
#define kprintk(fmt,args...) printk(KERN_DEBUG fmt,##args)
#define kprintf(fmt,args...) printf(fmt,##args)
#define kperror(str) perror(str)
#else
#define kprintk
#define kprintf
#define kperror
#endif


long g_old_sys_mkdir=0;
long * g_sys_call_table=NULL;
long g_oldcr0=0;
long hook=0;
long *hooktmp=0;
long ii=0;

//中断描述符表寄存器结构
//在x86中，idtr寄存器使得中断向量表可以存放在内存的任何位置，idtr寄存器有一个基地址
//和一个段限地址组成，高4字节为基地址，低2字节为段限地址。可以通过sidt指令获得idtr的内容。
struct _idtr{
    unsigned short  limit;
    unsigned int    base;
} __attribute__ ( ( packed ) );

//中断描述符表结构
//IDT基地址存放的是中断门，每个门8个字节，门描述符的格式参考Intel开发手册，其中，
//中断门是最低两个字节和最高两个字节构成了中断处理程序的地址。
struct _idt_descriptor
{
    unsigned short offset_low;
    unsigned short sel;
    unsigned char  none, flags;
    unsigned short offset_high;
} __attribute__((packed));

unsigned int close_cr(void)
{
    unsigned int cr0 = 0; //cr0控制寄存器
    unsigned int ret;
	//asm表示后面的代码为内嵌汇编，是__asm__的别名
	//volatile表示编译器不要优化代码，后面的指令保持原样，是__volatile__的别名
	//执行int 0x80后，系统调用的参数保存在寄存器中，eax传递的是系统调用号。
	//汇编代码，用于取出CR0寄存器的值
	//AT&T汇编代码使用小写字母，寄存器需要加前缀%
	//AT&T语法第一个为源操作数，第二个为目的操作数，方向从左到右
	//"movl %1, %0":"=r"(result):"m"(input) "=r"是表达式的说明和限制,(result)是每个操作数对应的C表达式
	//result前面的限制字符串是=r，=表示result是输出操作数
	//冒号后第一项对应的是%0，第二项对应的是%1
	//输入部分为空，也就是我们可以直接从CR0寄存器中取数；输出部分位cr0变量，a表示将cr0和eax相关联，
	//执行完后，cr0寄存器中的值就赋给了变量cr0.
	//在汇编中用 %序号 来代表这些输入/输出操作数，序号从 0 开始。为了与操作数区分开来，
	//寄存器用两个%引出，如：%%eax
    asm volatile ("movl %%cr0, %%eax"  
            : "=a"(cr0)
            );
    ret = cr0;
	////CR0的第16位是写保护位，0表示禁用写保护，1表示开启
    /*clear the 20th bit of CR0,*/
	//汇编代码，将修改后的CR0值写入CR0寄存器
    //输出部分为空，我们直接将结果输出到cr0寄存器中；输入部分为变量cr0，
	//它和eax寄存器相关联，执行完后，变量cr0的值就赋给了寄存器cr0.
    cr0 &= 0xfffeffff;//1111 1111 1111 1110 1111 1111 1111 1111禁用写保护位
    asm volatile ("movl %%eax, %%cr0"
            :
            : "a"(cr0)
            );
    return ret;//返回初始CR0值
}
//改回原CR0寄存器的值
void  open_cr(unsigned int oldval)
{
    asm volatile ("movl %%eax, %%cr0"
            :
            : "a"(oldval)
            );
}

long * get_sys_call_table(void)
{
	//中断向量表IDT的入口地址是通过IDTR寄存器来确定的
    struct _idt_descriptor * idt;
    struct _idtr idtr;
    unsigned int sys_call_off; // x80中断处理程序system_call 地址
    int sys_call_table=0;
    unsigned char* p;
    int i;
	// 从idtr中获得中断描述符(相当于中断号号表)的首地址
    //idtr寄存器的内容可以通过汇编指令sidt取出
	//通过sidt指令获得中断描述表寄存器内容放入idtr，通过idtr.base即可得到idt的基地址
    asm("sidt %0":"=m"(idtr));
    printk("addr of idtr: 0x%x\n", (unsigned int)&idtr);
	//由于每个中端描述符为8个字节，而软中断为int 0x80，据此获取系统调用中断即0x80的中断描述符的首地址
    //idt是获取到系统调用中断向量地址(即每一向量是中断服务程序的入口地址)，即0x80地址
    idt=(struct _idt_descriptor *)(idtr.base+8*0x80);
	//获取系统调用中断处理程序sys_call()的地址：
    sys_call_off=((unsigned int )(idt->offset_high<<16)|(unsigned int )idt->offset_low);
    printk("addr of idt 0x80: 0x%x\n", sys_call_off);
    p=(unsigned char *)sys_call_off;
	//获取系统调用表sys_call_table
    for (i=0; i<100; i++)
    {
		//通过反汇编sys_call函数，可以得知，只有在调用系统调用处使用了call指令，x86 call指令
		//的二进制格式为\xff\x14\x85，因此，我们可以从sys_call函数开始进行搜索，当出现\xff\x14\x85
		//指令的时候，即为call的地址，从而能得到存放sys_call_table的地址即当前地址+3，而系统调
		//用表即地址的内容，因此，获取系统调用表地址的实现过程就简单了。
        if (p[i]==0xff && p[i+1]==0x14 && p[i+2]==0x85)
        {
            sys_call_table=*(int*)((int)p+i+3);//查找系统调用表到sys_call_table的偏移量p+i +3表示3个指令码
            kprintk("addr of sys_call_table: 0x%x\n", sys_call_table);
			hook=p+i+3;
            return (long*)sys_call_table;
        }
    }
    return 0;
}


//asmlinkage long my_sys_getdents64(unsigned int fd, struct linux_dirent64 __user * dirent, unsigned int count)
//函数定义前加宏asmlinkage，表示这些函数通过堆栈而不是通过寄存器传递参数。
//告诉编译器仅从堆栈中获取该函数的参数
/*asmlinkage long my_sys_mkdir(const char __user *pathname, int mode)
{
    kprintk("can't' mkidr ^ ^\n");
    return -1;
}*/

//naked函数，即所谓的“裸函数”，对于这种函数，编译器不会生成任何函数入口代码和退出
//代码。这种函数一般应用在与操作系统内核相关的代码中，如中断处理函数、钩子函数等。
// 使用 naked 特性声明的函数，编译器将生成编码，而无需 prolog 和 epilog 代码。而
//一般性函数，编译器会主动加上很多prilog和epilog代码，还会做一些优化，有些是赘余的。
//naked 特性仅适用于 x86和ARM，并不用于 x64 。
__attribute__((naked)) void  test()
{
	int i=100;
	unsigned int table=0 ,*tmp;
	
	 __asm__ __volatile__ ("movl %%eax,%0 \n\t"

	                        :"=r"(i)
	                        :
	                        :
	                         ); 
	//输出调用号，即sys_call_table数组的下标。					 
    printk("number of sys_call_table: %x\n",i);
	printk("已经进入测试函数");
	
	tmp=ii+i;
	table=*tmp;//函数入口地址
	__asm__ __volatile__ ("movl %0,%%eax \n\t"
	                        :
	                        :"r"(i)
	                        :
	                         ); 						 							 
	__asm__ __volatile__ ("call %0 \n\t"
	
	                        :
	                        :"r"(table)
	                        :
	                         );  						 						 
}

void start_hook(void)
{
	//取到sys_call_table的地址
    ii=get_sys_call_table();
    kprintk("hookValue: 0x%x\n", hook);//hook=p+i+3;输出这个值方便调试
	//关闭cr0寄存器写保护
    g_oldcr0=close_cr(); 
	hooktmp=hook;
	kprintk("hookAddress: 0x%x\n", *hooktmp);//这个是输出hook值的地址即sys_call_table的地址    
	//*hooktmp 变量写入自己实现eax函数  在函数里面读取eax值
	//test函数地址写入hooktmp指向的内存,即sys_call_table所在内存地址变成了函数的地址
	*hooktmp=test;
	kprintk("testAddress: 0x%x\n", *hooktmp);//这个是输出test函数的地址
	mdelay(6000);
	*hooktmp=ii;
    open_cr(g_oldcr0);//打开cr0寄存器写保护
    //kprintk("new %08x %08x\n");//以十六进制的格式输出

}

int raider_init(void)
{
    kprintk("raider init\n");
    start_hook();
    return  0;
}

void raider_exit(void)
{
    kprintk("raider exit");
   
}


module_init(raider_init);
module_exit(raider_exit);

