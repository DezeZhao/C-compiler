int fac(int x)   //µÝ¹éº¯Êý
{
	int f;
	if(x <= 1)
	{
		f=1;
	}
	else
	{
		f=fac(x-1)*x;
	}
 
	return f;
}

int main()
{
	int a;
	int b;
	int c;
	a=3;
	a=fac(fac(a));
	return 0;
}