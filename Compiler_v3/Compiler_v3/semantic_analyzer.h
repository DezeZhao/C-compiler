#include"symbol_table.h"
#include<iomanip>
struct Quad {//��Ԫʽ
	int idx;//����
	string op;//�����(main,return,:=,j=,j>,j<......)
	string arg1;//����1
	string arg2;//����2
	string result;//���
};

struct Production
{
	string left;
	vector<string> right;
	bool operator==(const Production& prod) const {
		return this->left == prod.left && this->right == prod.right;
	}
};

struct Index {
	int tableIdx;//�ķ��������ڵı��ڷ��ű��е�����(���ű��Ƕ����ļ��ϣ�����ȫ�ַ��ű��������ű���ʱ�������ű�)
	int symbolIdx;//�ķ����������ڱ��е�����
};

struct GrammarSymbol {//�ķ����� һϵ������(�ۺ�����)
	string name;//�ķ�������(�ķ����ű���)
	string value;//�ķ�����ֵ(�����ķ�����const_int��Ӧ��ֵΪ������ĳ�����ֵ(��2) id---x)
	Index idx;//���� �����ڷ��ű��ж�λ����
	vector<int> trueList;//�����
	vector<int> falseList;//�ٳ���
	vector<int> nextList;//next����
	int quad;//����ֵ(��Ԫʽ���)
};


class SemanticAnalyzer {
private:
	ofstream quadsPrinter;//��Ԫʽ��ӡ
	vector<SymbolTable>symbolTables;//ȫ�ַ��ű���ʱ�������ű��������ű���
	vector<int>displayTable;//display��  �������ջ ����ָʾ��ǰxx���ű��ڷ��ű��е�����
	map<int,Quad>quads;//��Ԫʽ�� ����Ԫʽ��������˳�򱣴��ڸñ���  ����ٴ�ӡ���
	
	int mainFuncLine;//main�������ڵ�����
	int nextQuad;//��һ����Ԫʽ���
	void setQuad(const Quad& quad);//���γɵ���Ԫʽ���뵽��Ԫʽ����
	void backpatch(const vector<int>& backpatchQuad, int backpatchValue);//�����
	void createSymbolTable(TableType type, const string &name);//����xx���ű� display��xx���ű�����ѹջ ��ǰxx���ű������ͳ�ջ������
	int getNextQuad();
	string getArgName(const Index& idx, bool isReturnVar = false);//�˴������������ɴ����������xx���ű��ڷ��ű��������ͷ�������xx���ű����������
	void printQuads();//��ӡ��Ԫʽ��
public:
	bool semanticCheck(vector<GrammarSymbol> & grammarSymbolAttrStack,const Production &prod);//���﷨����ʱ����������
	SemanticAnalyzer();//���캯��  ����ȫ�ַ��ű� ����0 ������ʱ������ ����1
	~SemanticAnalyzer();//��������


};



 

