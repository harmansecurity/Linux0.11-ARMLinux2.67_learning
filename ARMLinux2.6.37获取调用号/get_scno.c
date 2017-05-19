#include <linux/module.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/sem.h>
#include <linux/msg.h>
#include <linux/shm.h>
#include <linux/stat.h>
#include <linux/syscalls.h>
#include <linux/mman.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/ipc.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
void get_syscall_num(void)
{
	//第一种方法的实现
	int i=100;
	asm("mov %0,r7 \n\t"        //arm里面顺序是反过来的
	     :"=r"(i)
	     :
	     );
	//输出调用号，即sys_call_table数组的下标。
	if(i!=0x03 && i != 0x67 && i!=0x4e && i!=0x121 && i!=0x12a && i!=0x12a){
		printk("%x ",i);
	}
	/*第二种方法的实现
	register unsigned int scno __asm__("r7");   //这句的意思是r7和scno是一回事
	if(scno!=0x03 && scno != 0x67 && scno!=0x4e && scno!=0x121 && scno!=0x12a && scno!=0x12a){
		printk("%x ",scno);
	}*/
}
