int a;
int b;
int program(int a,int b,int c)
{
	int i;
	int j;
	i=0; 	
	if(a>(b+c))
	{
		j=a+(b*c+1);
	}
	else
	{
		j=a;
	}
	while(i<=100)
	{
		i=j*2;
	}
	return i;
}

int demo(int a)
{
	a=a+2;
	return a*2;
}

void main(void)
{
	int xxx;
	int a;
	int b;
	int c;
	a=3;
	b=4;
	c=2;
	a=xxx(a,b,demo(c));
	return;
}