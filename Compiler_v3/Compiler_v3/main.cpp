#include"syntactic_analyzer.h"
#include "lexical_analyzer.h"

using namespace std;
int main()
{
	SyntacticAnalyzer sa;
	string codePath;
	cout << "������Դ���ļ���(*.cpp):";
	cin >> codePath;
	sa.build();
	
	sa.analyze(codePath);
	system("pause");
	return 0;
}