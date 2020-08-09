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

enum class SymbolType {//符号类型
	VAR,//变量
	FUNCTION,//函数(名)
	TEMP_VAR,//临时变量
	RETURN//返回值
};
struct Symbol {//此处可以看作变量 即变量名 变量值 变量类型 函数名也可看做变量
	SymbolType type; //符号属于哪种类型的变量(普通变量，函数名，临时变量)
	string varName;  //变量名(变量、常量、函数、临时变量名)
	string varValue; //变量值(常量值 或者空) 
	VarType varType; //变量的类型，若是函数则为函数返回值类型

	int formalParaNum = 0;//变量是函数名——形参的个数
	int funcTableIdx;//函数符号表在符号表中的索引

	int regIdx = -1;//寄存器索引 -1表示该符号不在寄存器中
};

struct Index {
	int tableIdx = -1;//文法符号所在的表在符号表中的索引(符号表是多个表的集合，比如全局符号表、函数符号表、临时变量符号表)
	int symbolIdx = -1;//文法符号在所在表中的索引
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
	SymbolTable(TableType type, const string& name);	//创建一个xx符号表
	string getName();//获取当前表名
	vector<Symbol>& getTable();//获取当前表
	int setSymbol(const Symbol& symbol);//向全局/函数符号表中添加符号 并返回该符号的索引
	int setSymbol(const string& value);//向临时变量表中添加符号 并返回该符号的索引
	int getSymbolIdx(const string& varName);//得到符号在某符号表中的索引
	Symbol& getSymbol(int idx);//得到某符号表中的符号 By 索引
	string getVarName(int idx);//得到某符号表中的符号（变量）的名字 By 索引
	SymbolType getSymbolType(int idx);//得到某符号表中的符号类型(VAR FUNC TEMP RETURN) By 索引
	void setSymbolType(int idx, SymbolType type);//通过索引修改该符号的符号类型  主要是设置为TEMP_VAR 便于在下次使用的时候直接取用
	VarType getVarType(int idx);//得到变量类型(int void)
};