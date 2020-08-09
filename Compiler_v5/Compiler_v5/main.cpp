#include"syntactic_analyzer.h"
#include "lexical_analyzer.h"

using namespace std;
int main(int argc, char* argv[])
{
	SyntacticAnalyzer sa;
	string codePath;
	string border = "********************************************************************************";
	cout << border << endl;
	for (int j = 0; j < 2; j++) {
		cout << "*";
		for (int i = 1; i < border.length() - 1; i++) {
			cout << " ";
		}
		cout << "*" << endl;
	}
	string welcom = "Welcom to C-like Compiler!";
	cout << "*";

	for (int i = 1; i < 27; i++) {
		cout << " ";
	}
	cout << welcom;
	for (int i = 1; i < 27; i++) {
		cout << " ";
	}
	cout << "*" << endl;
	for (int j = 0; j < 2; j++) {
		cout << "*";
		for (int i = 1; i < border.length() - 1; i++) {
			cout << " ";
		}
		cout << "*" << endl;
	}
	cout << border << endl;
	cout << "Tips:" << endl;
	cout << "1. Source data grammar_.txt is in folder SourceData in Root Dir." << endl;
	cout << "2. Generated data is in folder GeneratedData in Root Dir." << endl;
	cout << "3. If you would like to use customed grammar, you should create a file named \n"\
		"grammar_.txt in SourceData folder to REPLACE the original one. You'd better\nREMEMBER to backup it! But it may not work!" << endl;
	cout << "4. If you don't have folder GeneratedData, please create a folder named GeneratedData.\n"\
		"You can find all generated data of the C-like Compiler in it!" << endl;
	cout << "Now, you can enjoy it!" << endl;
	cout << endl;
	cout << "Please Enter Source Code File Name(like *.cpp):";
	cin >> codePath;
	sa.build();
	sa.analyze(codePath); 
	system("pause");
	return 0;
}