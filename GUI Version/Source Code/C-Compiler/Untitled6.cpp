int a;
int b;
int program(int a,int b)
{
	int i;
	int j;
	int a;
	i=0;
	if(a>b)
	{
		j = b-a;
		
	}
	else
	{
		j= b+a;
	}
	while(a<=100)
	{
		i=j*100;
	}
	return i;
}

int main(void)
{
	int a;
	int b;
	int c;
	a=3;
	b=4;
	c=2;
	c = program(a,b);
	return 0;
}
