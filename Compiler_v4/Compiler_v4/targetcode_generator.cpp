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
	ʹ��FIFO�㷨 ���мĴ�������
*/
int TargetCodeGenerator::getReg(const Index & idx){
	int allocReg = -1;
	vector<SymbolTable>& sb = *symbolTables_;
	if (queuePointer <= 9) {
		int front = regAllocQueue[queuePointer];//����Ԫ��
		if (regs[front].isFree == true) {//�Ĵ�������
			allocReg = front;//ֱ�ӷ���
		}
		else {//�Ĵ���ռ��
			
			//д���ڴ�
			string memAddr = getMemAddr(regs[front].idx);
			instructions.push_back({ "sw",regs[front].name,memAddr,"" });
			Index regIdx = regs[front].idx;
			sb[regIdx.tableIdx].getSymbol(regIdx.symbolIdx).regIdx = -1;//���ڼĴ���
			//�ٷ���
			allocReg = front;
		}
		queuePointer++;
		if (queuePointer == 10) {
			queuePointer = 0;//ָ�����Ԫ��
		}
	}
	return allocReg;
}

void TargetCodeGenerator::setActivityRecordHeader() {
	//�������¼ �ϵ�fp($sp)  ���ص�ַ($ra/$fp+4) ($sp=$sp+8) (��ʽ������Ԫ($sp+4...)) �ֲ����� ��ʱ����
	instructions.push_back({ "sw","$fp","($sp)","" });// sw $fp,0($sp)==>memory[$sp + 0]= $fp  �ϵ�fp���ڣ�$sp���ڴ���
	instructions.push_back({ "add","$fp","$sp","$0" });//�µ�fpָ��sp
	instructions.push_back({ "sw","$ra","4($sp)","" });//sw $ra,4($sp)==> memory[$sp + 4] = $ra;
	instructions.push_back({ "addi","$sp","$sp","8" });//�˴�$sp��Ҫ+8  $sp = $sp + 8;
}

void TargetCodeGenerator::addiImmToReg(const string & imm, const Index& idx){
	vector<SymbolTable>& sb = *symbolTables_;
	int reg = getReg(idx);//������üĴ���

	//�������������Ĵ���
	instructions.push_back({ "addi",regs[reg].name,"$0",imm });
	regs[reg].idx = idx;
	regs[reg].isFree = false;
	Symbol& s = sb[idx.tableIdx].getSymbol(idx.symbolIdx);
	//�÷������ĸ��Ĵ�����
	s.regIdx = reg;
}

void TargetCodeGenerator::loadMemToReg(const Index& idx){
	vector<SymbolTable>& sb = *symbolTables_;
	string memAddr = getMemAddr(idx);//ȡ�ڴ��ַ
	int reg = getReg(idx);//������üĴ���

	//���� �ڴ��е����� ���Ĵ���
	instructions.push_back({ "lw",regs[reg].name,memAddr,"" });
	regs[reg].idx = idx;
	regs[reg].isFree = false;
	Symbol& s = sb[idx.tableIdx].getSymbol(idx.symbolIdx);
	//�÷������ĸ���ʱ�Ĵ�����
	s.regIdx = reg;
}

string TargetCodeGenerator::getMemAddr(const Index& idx){
	vector<SymbolTable>& sb = *symbolTables_;
	string memAddr = "";
	if (idx.tableIdx == 0) {//ȫ�ֱ��� ����ʼ��ַΪ0x1000 0000
		int gpOffset = -1;
		for (auto iter = sb[0].getTable().begin(); iter != sb[0].getTable().end(); iter++) {
			if (iter->type == SymbolType::FUNCTION) {
				continue;//�Ǻ����� ����
			}
			//���Ǻ�����
			gpOffset++;
			if (iter - sb[0].getTable().begin() == idx.symbolIdx) {
				break;//�ҵ��봫���������һ�µķ�����
			}
		}
		memAddr = to_string(gpOffset * 4) + "($gp)";
	}
	else if (idx.tableIdx == 1) {//��ʱ���� ����ʼ��ַΪ0x1000 8000
		int gpOffset = 8192 + idx.symbolIdx;
		memAddr = to_string(gpOffset * 4) + "($gp)";
	}
	else {//�β� �ֲ����� ����ֵ
		//�õ���ǰ������������ȫ�ַ��ű��е�����
		int symbolIdx = sb[0].getSymbolIdx(sb[idx.tableIdx].getName());
		//����������ȫ�ַ��ű��еõ��ú�������
		Symbol &symbol  = sb[0].getSymbol(symbolIdx);

		if (idx.symbolIdx == 0) {//��������ֵ
			Symbol s = sb[idx.tableIdx].getSymbol(0);//�õ�����ֵ����
			if (s.varType == VarType::VOID) {
				memAddr = "";
			}
			else {//INT
				memAddr = "$v0";//$v0 $v1���ڴ�ź�������ֵ
			}
		}
		else {//�β� or �ֱ�
			if (idx.symbolIdx <= symbol.formalParaNum) {//�β�
				//$fpΪÿ�����¼����ʼָ�� ����Ϊÿ��������ĵ�0λΪ����ֵ  ��һ������������1��ʼ  ��Ҫ-1
				int fpOffset = -1 - symbol.formalParaNum + idx.symbolIdx;
				memAddr = to_string(fpOffset * 4) + "($fp)";
			}
			else {//�ֱ�
				int fpOffset = 1 + idx.symbolIdx - symbol.formalParaNum;//8($fp) ��ʼ��žֲ�����
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
	symbolTables_ = &symbolTables;//��ָ��ָ����������з��ű����ʼ��ַ
}

string TargetCodeGenerator::getOperand(const Index & idx){
	vector<SymbolTable>& sb = *symbolTables_;
	//��ȡ������Ϣ
	Symbol& s = sb[idx.tableIdx].getSymbol(idx.symbolIdx);
		
	if (s.regIdx == -1) {//���ڼĴ�����
		loadMemToReg(idx);//�ҵ���ʱ�������ڴ��е�ֵ  ������ص��Ĵ�����
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
	������мĴ��� 
	������Ҫע�� �Ĵ����еķ������������ʱ����  ��Ҫд���ڴ�
*/
void TargetCodeGenerator::clearRegs(){
	vector<SymbolTable>& sb = *symbolTables_;
	for (auto iter = regs.begin(); iter != regs.end(); iter++) {
		if (iter->isFree == true) {
			continue;//�Ĵ������� ����
		}
		Index idx = iter->idx;
		Symbol& s = sb[idx.tableIdx].getSymbol(idx.symbolIdx);//���ݼĴ����з������ڱ��������Ϣ �õ��üĴ�������
		s.regIdx = -1;//����  �÷��Ų��ڼĴ�����

		iter->isFree = true;//���� ���мĴ���
		iter->idx = { -1,-1 };//����

		//��ռĴ����������
		queuePointer = 0;//ָ��ָ�����

		if (s.type == SymbolType::TEMP_VAR) {//��ʱ����
			continue;//����
		}

		//����ʱ����(�ֲ�����) д�ض�Ӧ���ڴ��ַ��
		int regIdx = iter - regs.begin();
		string memAddr = getMemAddr(idx);//���ݷ��ŵ����� ���ڴ����ҵ���Ӧ��ַ
		//д���ڴ�
		instructions.push_back({ "sw",regs[regIdx].name,memAddr,"" });
	}
}

TargetCodeGenerator::TargetCodeGenerator() {
	targetcodePrinter.open("gen_data/target_code.txt");
	//ָ��10��Mars�е���ʱ�Ĵ���$t0~$t9
	for (int i = 0; i < 10; i++) {
		string name = "$t" + to_string(i);//$t0...
		string value = "";//��ʱΪ��
		Index idx = Index{ -1,-1 };
		bool isFree = true;//ȫ������
		regs.push_back({ name,value,idx,isFree });
		regAllocQueue.push_back(i);
	}
	queuePointer = 0;
	symbolTables_ = NULL;
	//lui rt,rd(imm);����mipsָ�ȫ�ֱ�����ռ64KB=65536B ����mars�Ĵ洢����0x10000000Ϊ��ʼ��ַ
	//$gp mars��31���Ĵ����ĵ�28�żĴ��� global pointer ȫ�ֱ���ָ��
	//�ٶ�ȫ�ֱ�������ʼ��ַΪ0x1000 0000 
	//�ٶ���ʱ��������ʼ��ַΪ0x1000 8000 
	instructions.push_back({ "lui","","$gp","0x1000" });
	//$sp mars�� 31���Ĵ����ĵ�29�żĴ��� stack pointer ��ջָ�� ʼ��ָ��ջ��
	//�˴���Ҫ����ָ��ָ��������ڴ��е���ʼ��ַ ����mars�Ĵ洢����0x10010000Ϊ�ڴ���ʼ��ַ
	instructions.push_back({ "lui","","$sp","0x1001" });
	//��������Ҫ��ת��main����
	instructions.push_back({ "j","","","main" });//main��ַ  ������ rd
}

TargetCodeGenerator::~TargetCodeGenerator(){
	targetcodePrinter.close();
}
