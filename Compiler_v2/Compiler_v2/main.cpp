#include"syntactic_analyzer.h"
#include "lexical_analyzer.h"

using namespace std;
int main()
{
	SyntacticAnalyzer sa;
	sa.build();
	sa.analyze();
	system("pause");
	return 0;
}