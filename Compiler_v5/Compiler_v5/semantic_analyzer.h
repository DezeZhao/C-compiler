#include"symbol_table.h"
#include"targetcode_generator.h"
#include<iomanip>
struct Quad {//四元式
	int idx;//索引
	string op;//运算符(main,return,:=,j=,j>,j<......)
	string arg1;//参数1
	string arg2;//参数2
	string result;//结果
};

struct Production
{
	string left;
	vector<string> right;
	bool operator==(const Production& prod) const {
		return this->left == prod.left && this->right == prod.right;
	}
};

struct GrammarSymbol {//文法符号 一系列属性(综合属性)
	string name;//文法符号名(文法符号本身)
	string value;//文法符号值(比如文法符号const_int对应的值为它代表的常量的值(如2) id---x)
	Index idx;//索引 用于在符号表中定位符号
	vector<int> trueList;//真出口
	vector<int> falseList;//假出口
	vector<int> nextList;//next出口
	vector<int> trueLabel;
	vector<int> falseLabel;
	vector<int> nextLabel;//目标代码生成需要
	int inst;//回填值(指令序号)
	int inst_= 0;//辅助
	int quad;//回填值(四元式序号)
};


class SemanticAnalyzer {
private:
	ofstream quadsPrinter;//四元式打印
	vector<SymbolTable>symbolTables;//全局符号表、临时变量符号表、函数符号表集合
	vector<int>displayTable;//display表  表的索引栈 用于指示当前xx符号表在符号表中的索引
	map<int, Quad>quads;//四元式表 将四元式按照生成顺序保存在该表中  最后再打印输出
	TargetCodeGenerator targetCodeGenerator;//目标代码生成类
	vector<int> labelStack;//用于 if 语句 while语句 开始结束标签标号
	int label = -1;//++label 初始化标签号为0  表示第0个标签  需要累加


	int mainFuncLine;//main函数所在的行数
	int nextQuad;//下一个四元式序号
	void setQuad(const Quad& quad);//将形成的四元式加入到四元式表中
	void backpatch(const vector<int>& backpatchQuad, int backpatchValue);//回填动作
	void createSymbolTable(TableType type, const string& name);//创建xx符号表 display表将xx符号表索引压栈 当前xx符号表分析完就出栈其索引
	int getNextQuad();
	string getArgName(const Index& idx, bool isReturnVar = false);//此处参数名称是由传入符号所在xx符号表在符号表中索引和符号所在xx符号表的索引决定
	void printQuads();//打印四元式表
public:
	bool semanticCheck(vector<GrammarSymbol>& grammarSymbolAttrStack, const Production& prod);//在语法分析时进行语义检查
	SemanticAnalyzer();//构造函数  创建全局符号表 索引0 创建临时变量表 索引1
	~SemanticAnalyzer();//析构函数


};