#include "targetcode_generator.h"

void TargetCodeGenerator::printInst() {
	for (auto iter = instructions.begin(); iter != instructions.end(); iter++) {
		/*targetcodePrinter << iter->op << " " << iter->rs << " " << iter->rt <<" "<< iter->rd << endl;*/
			targetcodePrinter << iter->op<<" ";
			if (iter->op == "j" || iter->op == "jal") {
				targetcodePrinter << iter->rd << endl;
			}
			else if (iter->op == "jr") {
				targetcodePrinter << iter->rs << endl;
			}
			else if(iter->rs ==":"){
				targetcodePrinter << iter->rs << endl;
			}
			else {
				if (iter->rs != "") {
					targetcodePrinter << iter->rs<<",";
				}
				if (iter->rt != "") {
					if ((iter->rt).find(')') == string::npos) {
						targetcodePrinter << iter->rt << ",";
					}
					else {
						targetcodePrinter << iter->rt;
					}
				}
				if (iter->rd != "") {
					targetcodePrinter << iter->rd;
				}
				targetcodePrinter << endl;
			}
	}
}
/*
	使用FIFO算法 进行寄存器分配
*/
int TargetCodeGenerator::getReg(const Index & idx){
	int allocReg = -1;
	vector<SymbolTable>& sb = *symbolTables_;
	if (queuePointer <= 9) {
		int front = regAllocQueue[queuePointer];//队首元素
		if (regs[front].isFree == true) {//寄存器空闲
			allocReg = front;//直接分配
		}
		else {//寄存器占用
			
			//写回内存
			string memAddr = getMemAddr(regs[front].idx);
			instructions.push_back({ "sw",regs[front].name,memAddr,"" });
			Index regIdx = regs[front].idx;
			sb[regIdx.tableIdx].getSymbol(regIdx.symbolIdx).regIdx = -1;//不在寄存器
			//再分配
			allocReg = front;
		}
		queuePointer++;
		if (queuePointer == 10) {
			queuePointer = 0;//指向队首元素
		}
	}
	return allocReg;
}

void TargetCodeGenerator::setActivityRecordHeader() {
	//创建活动记录 老的fp($sp)  返回地址($ra/$fp+4) ($sp=$sp+8) (形式参数单元($sp+4...)) 局部变量 临时变量
	instructions.push_back({ "sw","$fp","($sp)","" });// sw $fp,0($sp)==>memory[$sp + 0]= $fp  老的fp存在（$sp）内存中
	instructions.push_back({ "add","$fp","$sp","$0" });//新的fp指向sp
	instructions.push_back({ "sw","$ra","4($sp)","" });//sw $ra,4($sp)==> memory[$sp + 4] = $ra;
	instructions.push_back({ "addi","$sp","$sp","8" });//此处$sp还要+8  $sp = $sp + 8;
}

void TargetCodeGenerator::addiImmToReg(const string & imm, const Index& idx){
	vector<SymbolTable>& sb = *symbolTables_;
	int reg = getReg(idx);//分配可用寄存器

	//加载立即数进寄存器
	instructions.push_back({ "addi",regs[reg].name,"$0",imm });
	regs[reg].idx = idx;
	regs[reg].isFree = false;
	Symbol& s = sb[idx.tableIdx].getSymbol(idx.symbolIdx);
	//该符号在哪个寄存器中
	s.regIdx = reg;
}

void TargetCodeGenerator::loadMemToReg(const Index& idx){
	vector<SymbolTable>& sb = *symbolTables_;
	string memAddr = getMemAddr(idx);//取内存地址
	int reg = getReg(idx);//分配可用寄存器

	//加载 内存中的数据 进寄存器
	instructions.push_back({ "lw",regs[reg].name,memAddr,"" });
	regs[reg].idx = idx;
	regs[reg].isFree = false;
	Symbol& s = sb[idx.tableIdx].getSymbol(idx.symbolIdx);
	//该符号在哪个临时寄存器中
	s.regIdx = reg;
}

string TargetCodeGenerator::getMemAddr(const Index& idx){
	vector<SymbolTable>& sb = *symbolTables_;
	string memAddr = "";
	if (idx.tableIdx == 0) {//全局变量 其起始地址为0x1000 0000
		int gpOffset = -1;
		for (auto iter = sb[0].getTable().begin(); iter != sb[0].getTable().end(); iter++) {
			if (iter->type == SymbolType::FUNCTION) {
				continue;//是函数名 跳过
			}
			//不是函数名
			gpOffset++;
			if (iter - sb[0].getTable().begin() == idx.symbolIdx) {
				break;//找到与传入符号索引一致的符号了
			}
		}
		memAddr = to_string(gpOffset * 4) + "($gp)";
	}
	else if (idx.tableIdx == 1) {//临时变量 其起始地址为0x1000 8000
		int gpOffset = 8192 + idx.symbolIdx;
		memAddr = to_string(gpOffset * 4) + "($gp)";
	}
	else {//形参 局部变量 返回值
		//得到当前函数名符号在全局符号表中的索引
		int symbolIdx = sb[0].getSymbolIdx(sb[idx.tableIdx].getName());
		//根据索引在全局符号表中得到该函数符号
		Symbol &symbol  = sb[0].getSymbol(symbolIdx);

		if (idx.symbolIdx == 0) {//函数返回值
			Symbol s = sb[idx.tableIdx].getSymbol(0);//得到返回值符号
			if (s.varType == VarType::VOID) {
				memAddr = "";
			}
			else {//INT
				memAddr = "$v0";//$v0 $v1用于存放函数返回值
			}
		}
		else {//形参 or 局变
			if (idx.symbolIdx <= symbol.formalParaNum) {//形参
				//$fp为每个活动记录的起始指针 又因为每个函数表的第0位为返回值  第一个参数索引从1开始  故要-1
				int fpOffset = -1 - symbol.formalParaNum + idx.symbolIdx;
				memAddr = to_string(fpOffset * 4) + "($fp)";
			}
			else {//局变
				int fpOffset = 1 + idx.symbolIdx - symbol.formalParaNum;//8($fp) 开始存放局部变量
				memAddr = to_string(fpOffset * 4) + "($fp)";
			}
		}
	}
	return memAddr;
}

void TargetCodeGenerator::incStackPointer(int i){
	instructions.push_back({ "addi","$sp","$sp",to_string(i * 4) });
}

void TargetCodeGenerator::getSymbolTables(vector<SymbolTable>& symbolTables){
	symbolTables_ = &symbolTables;//该指针指向语义分析中符号表的起始地址
}

string TargetCodeGenerator::getOperand(const Index & idx){
	vector<SymbolTable>& sb = *symbolTables_;
	//获取符号信息
	Symbol& s = sb[idx.tableIdx].getSymbol(idx.symbolIdx);
		
	if (s.regIdx == -1) {//不在寄存器中
		loadMemToReg(idx);//找到临时变量在内存中的值  将其加载到寄存器中
	}
	string operand = "$t" + to_string(s.regIdx);//$t0...

	return operand;
}

int TargetCodeGenerator::setInstruction(const Instruction& inst){
	this->instructions.push_back(inst);
	
	return this->instructions.size() - 1;
}

void TargetCodeGenerator::backpatch_(const vector<int>& backpatchInst, const int& backpatchValue, string labelId){
	for (auto iter = backpatchInst.begin(); iter != backpatchInst.end(); iter++) {
		instructions[*iter].rd = instructions[backpatchValue].op;
	}
}

Instruction TargetCodeGenerator::getInst(const int& inst){
	return instructions[inst];
}
/*
	清空所有寄存器 
	但是需要注意 寄存器中的符号如果不是临时变量  需要写回内存
*/
void TargetCodeGenerator::clearRegs(){
	vector<SymbolTable>& sb = *symbolTables_;
	for (auto iter = regs.begin(); iter != regs.end(); iter++) {
		if (iter->isFree == true) {
			continue;//寄存器空闲 跳过
		}
		Index idx = iter->idx;
		Symbol& s = sb[idx.tableIdx].getSymbol(idx.symbolIdx);//根据寄存器中符号所在表的索引信息 得到该寄存器符号
		s.regIdx = -1;//重置  该符号不在寄存器了

		iter->isFree = true;//重置 空闲寄存器
		iter->idx = { -1,-1 };//重置

		//清空寄存器分配队列
		queuePointer = 0;//指针指向队首

		if (s.type == SymbolType::TEMP_VAR) {//临时变量
			continue;//跳过
		}

		//非临时变量(局部变量) 写回对应的内存地址处
		int regIdx = iter - regs.begin();
		string memAddr = getMemAddr(idx);//根据符号的索引 在内存中找到对应地址
		//写回内存
		instructions.push_back({ "sw",regs[regIdx].name,memAddr,"" });
	}
}

TargetCodeGenerator::TargetCodeGenerator() {
	targetcodePrinter.open("gen_data/target_code.txt");
	//指定10个Mars中的临时寄存器$t0~$t9
	for (int i = 0; i < 10; i++) {
		string name = "$t" + to_string(i);//$t0...
		string value = "";//暂时为空
		Index idx = Index{ -1,-1 };
		bool isFree = true;//全部空闲
		regs.push_back({ name,value,idx,isFree });
		regAllocQueue.push_back(i);
	}
	queuePointer = 0;
	symbolTables_ = NULL;
	//lui rt,rd(imm);根据mips指令，全局变量区占64KB=65536B 按照mars的存储方法0x10000000为起始地址
	//$gp mars中31个寄存器的第28号寄存器 global pointer 全局变量指针
	//假定全局变量的起始地址为0x1000 0000 
	//假定临时变量的起始地址为0x1000 8000 
	instructions.push_back({ "lui","","$gp","0x1000" });
	//$sp mars中 31个寄存器的第29号寄存器 stack pointer 堆栈指针 始终指向栈顶
	//此处需要将该指针指向程序在内存中的起始地址 按照mars的存储方法0x10010000为内存起始地址
	instructions.push_back({ "lui","","$sp","0x1001" });
	//接下来需要跳转到main函数
	instructions.push_back({ "j","","","main" });//main地址  立即数 rd
}

TargetCodeGenerator::~TargetCodeGenerator(){
	targetcodePrinter.close();
}
