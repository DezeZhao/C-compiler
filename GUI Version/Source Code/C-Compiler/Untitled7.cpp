int a;
int b;
int program(int a,int b,int c)
{
	int i;
	int j;
	i=0; 	
	if(a>(b+c))//��������ifelse��if��� ��ռĴ���
	{
		j=a+(b*c+1);
		/*�������뵥if��� ��ռĴ���
		���Ƕ���ע��*/
		if(a>b)
		{
			i = j;
			j = a-b;/*�����˳���if��� ��ռĴ���*/
		}
		j = a+b-c;
		//�����˳�ifelse ��if��� ��ռĴ���
	}
	//��������if else ��else ��ռĴ���
	else
	{
		j=a;
		//�����˳�else��� ��ռĴ���
	}
	//����while ǰ ��ռĴ���
	while(i<=100)
	{
		i=j*100;
		//�˳�while���  ��ռĴ���
	}
	return i;
	//�˳�����ǰ ��ռĴ���
}

int demo(int a)
{
	a=a+2;
	return ;
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
