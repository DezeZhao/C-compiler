#pragma once
#include"symbol_table.h"

struct Register{//�Ĵ���
	string name;//�Ĵ�����
	string value = "";//�Ĵ���ֵ
	Index idx = { -1,-1 };//����������
	bool isFree = true;//�Ƿ����
};

struct Instruction{//mipsָ��
	string op;//������
	string rs;//������1
	string rt;//������2
	string rd;//������3
};

class TargetCodeGenerator {
private:
	vector<Register> regs;
	ofstream targetcodePrinter;//Ŀ������ӡ
	vector<Instruction> instructions;//���ɵ�mipsָ������
	vector<SymbolTable> *symbolTables_;//��������еķ��ű�ĸ��� ����Ŀ���������
	vector<int> regAllocQueue;//�Ĵ����������(�Ĵ���ռ��)
	int queuePointer;//�Ĵ�����������ƶ�ָ��  ģ�����
	int getReg(const Index & idx);//�õ��������ʱ�Ĵ������
	
public:
	void printInst();//��ӡָ��
	void setActivityRecordHeader();//�������¼��ͷ��
	void addiImmToReg(const string& imm, const Index& idx);//�����������ص��Ĵ���
	void loadMemToReg(const Index& idx);//�����ڴ��е������Ĵ���
	string getMemAddr(const Index& idx);//����ڴ��ַ
	void incStackPointer(int i);//�ƶ�ջ��ָ��
	void getSymbolTables(vector<SymbolTable>& symbolTables);//��ָ��ָ����������з��ű�ĵ�ַ 
	string getOperand(const Index& idx);//��ȡ������
	int setInstruction(const Instruction& inst);//��instructions�������ָ�� ����ָ�����
	void backpatch_(const vector<int>& backpatchInst, const int& backpatchValue,string labelId = "");//����ָ��
	Instruction getInst(const int& inst);//�õ�instructions��ĳ��ָ��
	void clearRegs();//��ռĴ��������ҽ�����ʱ����д���ڴ�
	TargetCodeGenerator();
	~TargetCodeGenerator();
};