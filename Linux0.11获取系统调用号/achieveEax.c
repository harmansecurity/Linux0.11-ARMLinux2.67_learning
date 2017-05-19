#define __LIBRARY__
#include<unistd.h>
int achieveEax(void)
{
	int i=100;
	__asm__ 
	(
	"movl %%eax,%0 \n\t"        //arm里面顺序是反过来的                        
	:"=r"(i)
	:
	); 
	//输出调用号，即sys_call_table数组的下标。
	if(i!=0x1d){		
		printk("%x ",i);
	}
	return i;
}