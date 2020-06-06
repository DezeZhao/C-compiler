#pragma once
#include<iostream>
#include<set>
#include<map>
#include<vector>
#include<fstream>
#include<string>
#include<stack>
#include<queue>
using namespace std;

enum class VarType {
	INT,//int
	VOID//void
};

enum class SymbolType {//��������
	VAR,//����
	FUNCTION,//����(��)
	TEMP_VAR,//��ʱ����
	RETURN//����ֵ
};
struct Symbol {//�˴����Կ������� �������� ����ֵ �������� ������Ҳ�ɿ�������
	SymbolType type; //���������������͵ı���(��ͨ����������������ʱ����)
	string varName;  //������(��������������������ʱ������)
	string varValue; //����ֵ(����ֵ ���߿�) 
	VarType varType; //���������ͣ����Ǻ�����Ϊ��������ֵ����

	int formalParaNum = 0;//�����Ǻ����������βεĸ���
	int funcTableIdx;//�������ű��ڷ��ű��е�����

	int regIdx = -1;//�Ĵ������� -1��ʾ�÷��Ų��ڼĴ�����
};

struct Index {
	int tableIdx = -1;//�ķ��������ڵı��ڷ��ű��е�����(���ű��Ƕ����ļ��ϣ�����ȫ�ַ��ű��������ű���ʱ�������ű�)
	int symbolIdx = -1;//�ķ����������ڱ��е�����
};

enum class TableType {
	GLOBAL,
	TEMPVAR,
	FUNCTION
};

class SymbolTable {
private:
	vector<Symbol> symbolTable;
	string name;
	TableType type;

public:
	SymbolTable(TableType type, const string& name);	//����һ��xx���ű�
	string getName();//��ȡ��ǰ����
	vector<Symbol>& getTable();//��ȡ��ǰ��
	int setSymbol(const Symbol& symbol);//��ȫ��/�������ű�����ӷ��� �����ظ÷��ŵ�����
	int setSymbol(const string& value);//����ʱ����������ӷ��� �����ظ÷��ŵ�����
	int getSymbolIdx(const string& varName);//�õ�������ĳ���ű��е�����
	Symbol& getSymbol(int idx);//�õ�ĳ���ű��еķ��� By ����
	string getVarName(int idx);//�õ�ĳ���ű��еķ��ţ������������� By ����
	SymbolType getSymbolType(int idx);//�õ�ĳ���ű��еķ�������(VAR FUNC TEMP RETURN) By ����
	void setSymbolType(int idx, SymbolType type);//ͨ�������޸ĸ÷��ŵķ�������  ��Ҫ������ΪTEMP_VAR �������´�ʹ�õ�ʱ��ֱ��ȡ��
	VarType getVarType(int idx);//�õ���������(int void)
};