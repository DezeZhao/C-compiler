#include"symbol_table.h"

SymbolTable::SymbolTable(TableType type, const string& name)
{
	this->type = type;
	this->name = name;
}

string SymbolTable::getName()
{
	return this->name;
}
/*
	�������ű����ȫ�ַ��ű���ӷ���
*/
int SymbolTable::setSymbol(const Symbol& symbol)
{
	this->symbolTable.push_back(symbol);
	return this->symbolTable.size() - 1;
}

/*
	����ʱ�����ӽ���ʱ�������ű�
*/
int SymbolTable::setSymbol(const string& value)
{
	string name = "T" + std::to_string(this->symbolTable.size());
	Symbol s;
	s.type = SymbolType::TEMP_VAR;
	s.varName = name;
	s.varType = VarType::INT;
	s.varValue = value;
	this->symbolTable.push_back(s);
	return this->symbolTable.size() - 1;
}
/*
	�õ�������xxx���ű��е����� ͨ��������
*/
int SymbolTable::getSymbolIdx(const string& varName)
{
	for (auto iter = this->symbolTable.begin(); iter != this->symbolTable.end(); iter++) {
		if (iter->varName == varName) {
			return iter - this->symbolTable.begin();//�ҵ���
		}
	}
	return -1;//û�ҵ�
}
/*
	���������õ�XX���ű��еķ���
*/
Symbol& SymbolTable::getSymbol(int idx)
{
	return this->symbolTable[idx];
}
/*
	���������õ����ŵ�����(������ ������....)
*/
string SymbolTable::getVarName(int idx)
{
	return this->symbolTable[idx].varName;
}
/*
	���������õ����ű���ĳ�������ķ�������(VAR,FUNC,TEMP)����ʲô����
*/
SymbolType SymbolTable::getSymbolType(int idx)
{
	return this->symbolTable[idx].type;
}
void SymbolTable::setSymbolType(int idx, SymbolType type){
	this->symbolTable[idx].type = type;
}
/*
	���������õ����ű���ĳ�������ķ��Ŵ���ı�������(int void)����ʲô��������
*/
VarType SymbolTable::getVarType(int idx)
{
	return this->symbolTable[idx].varType;
}
