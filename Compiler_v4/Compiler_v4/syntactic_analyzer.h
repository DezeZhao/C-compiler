#pragma once
#include<iostream>
#include<set>
#include<map>
#include<vector>
#include<fstream>
#include<string>
#include<stack>
#include<queue>
#include"semantic_analyzer.h"
using namespace std;

enum class Action {
	REDUCE = 0,//��Լ
	SHIFT,//�ƽ�
	ACCEPT,//����
	NOACTION//�޶���
};


struct Grammar {
	string left;
	vector<string> right;
};

struct LR_0_Item {//LR0 ��Ŀ ����A-->B��
	size_t prodPos;//����ʽλ��/���
	size_t pointPos;//Բ���λ��

	bool operator<(const LR_0_Item& lr0Item) const {//����< setĬ�ϻ�������Ԫ�ؽ������� ����ȥ��Ҳ��Ҫ
		return this->prodPos < lr0Item.prodPos
			|| (this->prodPos == lr0Item.prodPos && this->pointPos < lr0Item.pointPos);
	}
};

struct LR_1_Item :public LR_0_Item {
	//LR_0_Item lr0Item;//LR0��Ŀ
	set<string> lookaheadStr;//չ����
	LR_1_Item(size_t prodPos, size_t pointPos, set<string> lookaheadStr) {
		this->prodPos = prodPos;
		this->pointPos = pointPos;
		this->lookaheadStr = lookaheadStr;
	}

	bool operator==(const LR_1_Item& lr1Item) const {//����== ��vector��Ҫ
		return this->prodPos == lr1Item.prodPos &&
			this->pointPos == lr1Item.pointPos &&
			this->lookaheadStr == lr1Item.lookaheadStr;//չ����Ҳ����һ�������ж�����LR1��Ŀ��ͬ
	}
	bool operator<(const LR_1_Item& lr1Item) const {//����< set��find������Ҫ insertҲ����Ҫ
		return this->prodPos < lr1Item.prodPos
			|| (this->prodPos == lr1Item.prodPos && this->pointPos < lr1Item.pointPos)
			|| (this->prodPos == lr1Item.prodPos && this->pointPos == lr1Item.pointPos && this->lookaheadStr < lr1Item.lookaheadStr);
		//չ����Ҳ����һ�������ж�����LR1��Ŀ��ͬ
	}
};

struct ActionGoto {
	Action action;//����
	int  nextState;//��һ״̬
};

struct StateSymbol {//LR1�������е�״̬���ķ�����
	int state;
	string symbol;
	bool operator<(const StateSymbol& ss) const {//�˴���������<����ΪmapĬ�ϻ�������Ԫ������
		return this->state < ss.state || (this->state == ss.state && this->symbol > ss.symbol);
	}
};

class SyntacticAnalyzer {
private:
	//��Ա����
	string programStart;
	SemanticAnalyzer semanticAnalyzer;
	vector<Production> prods;//���� ����ʽ�ļ���
	set<string>terminalSymbols;//�ս��
	set<string>nonterminalSymbols;//���ս������
	map<string, set<string>>firstSet;//First ����
	map<string, set<string>>followSet;//Follow ����
	vector<LR_0_Item>lr0Items;//���� LR0��Ŀ 
	vector<set<LR_1_Item>> lr1ItemsNormalFamily;//LR1��Ŀ���淶��
	map<StateSymbol, ActionGoto>LR1AnalysisTable;//LR1 ������
	set<LR_1_Item>lr1ItemClosure0;//��ʼ��Ŀ���հ�
	LR_1_Item lr1Item0 = LR_1_Item{ 0,0,set<string>{} };//�����ʼ��
	vector<int>stateSequenceStack;//״̬����ջ
	vector<string>grammarSymbolStack;//�ķ�����ջ
	vector<GrammarSymbol>grammarSymbolAttrStack;//�ķ���������ջ ���﷨������ʱ��������Լ���
	ofstream printer;

	//����
	bool getProd(const string&);//���ļ��õ��ķ��Ĳ���ʽ����
	void getFirstSet();//�õ�first����
	void getExtendGrammar();//�õ��ع��ķ�
	void getFollowSet();//�õ���Follow����
	void getLR0Items();//�õ�����LR0��Ŀ
	void getLR1ItemClosure0();//�õ���ʼ��Ŀ����Ŀ���հ�
	void getLR1ItemsNormalFamily();//����LR1��Ŀ���淶��
	set<string> getProdFirstSet(const vector<string>&);//�õ�����ʽ����first����
	set<LR_1_Item> go(const set<LR_1_Item>&);//go����  �����γ�ʶ���ǰ׺��DFA 

	//��ӡ
	void printGrammarSymbols();//��ӡ�ķ�����
	void printProductions();//��ӡ������������
	void printLR0Item(const LR_0_Item&);//��ӡһ��LR0��Ŀ
	void printLR0Items();//��ӡ����LR0��Ŀ
	void printLR1ItemsNormalFamily();//��ӡLR1��Ŀ���淶��
	void printLR1AnalysisTabel();//��ӡLR1�﷨������
	void printAnalysisProcess(int, const vector<string>&, const vector<int>&, const Production&);//��ӡ��������
	void funcPrint();//��ӡ�����ܺ���

	//���ص�print���� ��������Ĵ�ӡ����
	void print(const vector<string>&);
	void print(const set<string>&);
	void print(const map<string, set<string>>&);
	void print(const vector<Production>&);

	//���ߺ���
	bool isNonTerminalSymbol(const string& symbol);//�ж��Ƿ�Ϊ���ս��
	vector<string> split(const string&, const string&);//�ָ����ʽ�Ҳ�

public:
	void build();
	void analyze(const string& codePath);
};