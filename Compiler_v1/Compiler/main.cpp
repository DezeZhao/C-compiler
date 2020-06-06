#include"syntactic_analyzer.h"
#include "lexical_analyzer.h"

using namespace std;
int main()
{
	/*SyntacticAnalyzer sa;
	sa.test();*/

	LexicalAnalyzer la;
	string codeFilename = "raw_data/test.cpp";
	string resultFilename = "gen_data/lexical_result.csv";
	if (la.openFile(codeFilename, resultFilename))
	{
		la.packBuffer();
		while (true) {
			Word word = la.getWord();
			if (word.type == LEXICAL_TYPE::_EOF)	break;
		}
	}
	system("pause");
	return 0;
}