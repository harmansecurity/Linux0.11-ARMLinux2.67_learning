#define __LIBRARY__
#include<unistd.h>
int achieveEax(void)
{
	int i=100;
	__asm__ 
	(
	"movl %%eax,%0 \n\t"        //arm����˳���Ƿ�������                        
	:"=r"(i)
	:
	); 
	//������úţ���sys_call_table������±ꡣ
	if(i!=0x1d){		
		printk("%x ",i);
	}
	return i;
}