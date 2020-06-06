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
	函数符号表或者全局符号表添加符号
*/
int SymbolTable::setSymbol(const Symbol& symbol)
{
	this->symbolTable.push_back(symbol);
	return this->symbolTable.size() - 1;
}

/*
	将临时变量加进临时变量符号表
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
	得到符号在xxx符号表中的索引 通过符号名
*/
int SymbolTable::getSymbolIdx(const string& varName)
{
	for (auto iter = this->symbolTable.begin(); iter != this->symbolTable.end(); iter++) {
		if (iter->varName == varName) {
			return iter - this->symbolTable.begin();//找到了
		}
	}
	return -1;//没找到
}
/*
	根据索引得到XX符号表中的符号
*/
Symbol& SymbolTable::getSymbol(int idx)
{
	return this->symbolTable[idx];
}
/*
	根据索引得到符号的名字(变量名 函数名....)
*/
string SymbolTable::getVarName(int idx)
{
	return this->symbolTable[idx].varName;
}
/*
	根据索引得到符号表中某个索引的符号类型(VAR,FUNC,TEMP)——什么变量
*/
SymbolType SymbolTable::getSymbolType(int idx)
{
	return this->symbolTable[idx].type;
}
void SymbolTable::setSymbolType(int idx, SymbolType type){
	this->symbolTable[idx].type = type;
}
/*
	根据索引得到符号表中某个索引的符号代表的变量类型(int void)——什么数据类型
*/
VarType SymbolTable::getVarType(int idx)
{
	return this->symbolTable[idx].varType;
}
