int e;
int d;
int program(int a,int b,int c)
{
	int i;
	int j;
	e=4;
	d=5;
	i=0; 
	j=d+e;
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
	int a;
	int b;
	int c;
	a=3;
	b=4;
	c=2;
	a=program(a,b,demo(c));
	return;
}
