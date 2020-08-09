#pragma once
#include"symbol_table.h"

struct Register{//寄存器
	string name;//寄存器名
	string value = "";//寄存器值
	Index idx = { -1,-1 };//表及符号索引
	bool isFree = true;//是否空闲
};

struct Instruction{//mips指令
	string op;//操作码
	string rs;//操作数1
	string rt;//操作数2
	string rd;//操作数3
};

class TargetCodeGenerator {
private:
	vector<Register> regs;
	ofstream targetcodePrinter;//目标代码打印
	vector<Instruction> instructions;//生成的mips指令序列
	vector<SymbolTable> *symbolTables_;//语义分析中的符号表的复制 用于目标代码生成
	vector<int> regAllocQueue;//寄存器分配队列(寄存器占用)
	int queuePointer;//寄存器分配队列移动指针  模拟队列
	int getReg(const Index & idx);//得到分配的临时寄存器编号
	
public:
	void printInst();//打印指令
	void setActivityRecordHeader();//创建活动记录的头部
	void addiImmToReg(const string& imm, const Index& idx);//将立即数加载到寄存器
	void loadMemToReg(const Index& idx);//加载内存中的数进寄存器
	string getMemAddr(const Index& idx);//获得内存地址
	void incStackPointer(int i);//移动栈顶指针
	void getSymbolTables(vector<SymbolTable>& symbolTables);//将指针指向语义分析中符号表的地址 
	string getOperand(const Index& idx);//获取操作数
	int setInstruction(const Instruction& inst);//向instructions里面添加指令 返回指令序号
	void backpatch_(const vector<int>& backpatchInst, const int& backpatchValue,string labelId = "");//回填指令
	Instruction getInst(const int& inst);//得到instructions的某条指令
	void clearRegs();//清空寄存器，并且将非临时变量写到内存
	TargetCodeGenerator();
	~TargetCodeGenerator();
};