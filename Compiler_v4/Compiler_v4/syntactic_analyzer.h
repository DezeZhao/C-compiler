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
	REDUCE = 0,//规约
	SHIFT,//移进
	ACCEPT,//接受
	NOACTION//无动作
};


struct Grammar {
	string left;
	vector<string> right;
};

struct LR_0_Item {//LR0 项目 诸如A-->B·
	size_t prodPos;//产生式位置/序号
	size_t pointPos;//圆点的位置

	bool operator<(const LR_0_Item& lr0Item) const {//重载< set默认会对里面的元素进行排序 而且去重也需要
		return this->prodPos < lr0Item.prodPos
			|| (this->prodPos == lr0Item.prodPos && this->pointPos < lr0Item.pointPos);
	}
};

struct LR_1_Item :public LR_0_Item {
	//LR_0_Item lr0Item;//LR0项目
	set<string> lookaheadStr;//展望串
	LR_1_Item(size_t prodPos, size_t pointPos, set<string> lookaheadStr) {
		this->prodPos = prodPos;
		this->pointPos = pointPos;
		this->lookaheadStr = lookaheadStr;
	}

	bool operator==(const LR_1_Item& lr1Item) const {//重载== ，vector需要
		return this->prodPos == lr1Item.prodPos &&
			this->pointPos == lr1Item.pointPos &&
			this->lookaheadStr == lr1Item.lookaheadStr;//展望串也必须一样才能判断两个LR1项目相同
	}
	bool operator<(const LR_1_Item& lr1Item) const {//重载< set的find函数需要 insert也会需要
		return this->prodPos < lr1Item.prodPos
			|| (this->prodPos == lr1Item.prodPos && this->pointPos < lr1Item.pointPos)
			|| (this->prodPos == lr1Item.prodPos && this->pointPos == lr1Item.pointPos && this->lookaheadStr < lr1Item.lookaheadStr);
		//展望串也必须一样才能判断两个LR1项目相同
	}
};

struct ActionGoto {
	Action action;//动作
	int  nextState;//下一状态
};

struct StateSymbol {//LR1分析表中的状态和文法符号
	int state;
	string symbol;
	bool operator<(const StateSymbol& ss) const {//此处必须重载<，因为map默认会对里面的元素排序
		return this->state < ss.state || (this->state == ss.state && this->symbol > ss.symbol);
	}
};

class SyntacticAnalyzer {
private:
	//成员变量
	string programStart;
	SemanticAnalyzer semanticAnalyzer;
	vector<Production> prods;//单个 产生式的集合
	set<string>terminalSymbols;//终结符
	set<string>nonterminalSymbols;//非终结符集合
	map<string, set<string>>firstSet;//First 集合
	map<string, set<string>>followSet;//Follow 集合
	vector<LR_0_Item>lr0Items;//所有 LR0项目 
	vector<set<LR_1_Item>> lr1ItemsNormalFamily;//LR1项目集规范族
	map<StateSymbol, ActionGoto>LR1AnalysisTable;//LR1 分析表
	set<LR_1_Item>lr1ItemClosure0;//初始项目集闭包
	LR_1_Item lr1Item0 = LR_1_Item{ 0,0,set<string>{} };//必须初始化
	vector<int>stateSequenceStack;//状态序列栈
	vector<string>grammarSymbolStack;//文法符号栈
	vector<GrammarSymbol>grammarSymbolAttrStack;//文法符号属性栈 在语法分析的时候进行属性计算
	ofstream printer;

	//分析
	bool getProd(const string&);//从文件得到文法的产生式集合
	void getFirstSet();//得到first集合
	void getExtendGrammar();//得到拓广文法
	void getFollowSet();//得到所Follow集合
	void getLR0Items();//得到所有LR0项目
	void getLR1ItemClosure0();//得到初始项目的项目集闭包
	void getLR1ItemsNormalFamily();//构造LR1项目集规范族
	set<string> getProdFirstSet(const vector<string>&);//得到产生式串的first集合
	set<LR_1_Item> go(const set<LR_1_Item>&);//go函数  用于形成识别活前缀的DFA 

	//打印
	void printGrammarSymbols();//打印文法符号
	void printProductions();//打印单个产生集合
	void printLR0Item(const LR_0_Item&);//打印一个LR0项目
	void printLR0Items();//打印所有LR0项目
	void printLR1ItemsNormalFamily();//打印LR1项目集规范族
	void printLR1AnalysisTabel();//打印LR1语法分析表
	void printAnalysisProcess(int, const vector<string>&, const vector<int>&, const Production&);//打印分析过程
	void funcPrint();//打印功能总函数

	//重载的print函数 辅助上面的打印函数
	void print(const vector<string>&);
	void print(const set<string>&);
	void print(const map<string, set<string>>&);
	void print(const vector<Production>&);

	//工具函数
	bool isNonTerminalSymbol(const string& symbol);//判断是否为非终结符
	vector<string> split(const string&, const string&);//分割产生式右部

public:
	void build();
	void analyze(const string& codePath);
};