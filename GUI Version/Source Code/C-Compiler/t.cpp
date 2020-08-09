int a;
int b;
int program(int a,int b,int c)
{
	int i;
	int j;
	i=0; 	
	//即将进入ifelse的if语句 清空寄存器
	if(a>(b+c))
	{
		j=a+(b*c+1);
		//即将进入单if语句 清空寄存器
		if(a>b)
		{
			i = j;
			j = a-b;
			//即将退出单if语句 清空寄存器
		}
		j = a+b-c;
		//即将退出ifelse 的if语句 清空寄存器
	}
	//即将进入if else 的else 清空寄存器
	else
	{
		j=a;
		//即将退出else语句 清空寄存器
	}
	//进入while 前 清空寄存器
	while(i<=100)
	{
		i=j*20;
		j=j+1;
		//退出while语句  清空寄存器
	}
	return i;
	//退出函数前 清空寄存器
}

int demo(int a)
{
	a=a+2;
	return a*2;
}

void main(void)
{
	int a;
	int b;
	int c;
	a=3;
	b=4;
	c=2;
	a=program(a,b,demo(c));
	return;
}
