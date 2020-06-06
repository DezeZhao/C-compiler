#include "semantic_analyzer.h"

/*
	初始化  创建两个表
*/
SemanticAnalyzer::SemanticAnalyzer() {
	quadsPrinter.open("gen_data/quads.txt");
	symbolTables.push_back(SymbolTable(TableType::GLOBAL, "global"));//全局符号表 [0]
	symbolTables.push_back(SymbolTable(TableType::TEMPVAR, "temp"));//临时变量符号表 [1] //辅助作用 并无标识符定义 不会加在display表中
	displayTable.push_back(0);//一开始分析的表肯定是全局符号表(0)
	nextQuad = 0;//下一个四元式序号
	mainFuncLine = -1;// main函数的行数初始化为-1

	//目标代码生成 初始化
	targetCodeGenerator.getSymbolTables(symbolTables);
}
SemanticAnalyzer::~SemanticAnalyzer() {
	quadsPrinter.close();
}
/*
	得到下一个四元式的序号 从1开始
*/
int SemanticAnalyzer::getNextQuad() {
	return ++nextQuad;
}
string SemanticAnalyzer::getArgName(const Index& idx, bool isReturnVal) {
	string argName = symbolTables[idx.tableIdx].getVarName(idx.symbolIdx);
	if (isReturnVal == true) {//是函数返回值
		int symbolIdx = symbolTables[0].getSymbolIdx(symbolTables[idx.tableIdx].getName());
		Symbol& symbol = symbolTables[0].getSymbol(symbolIdx);
		argName = symbol.varName + "_param_" + to_string(idx.symbolIdx);//函数名_param_形参序号
	}
	else {//不是
		if (SymbolType::VAR == symbolTables[idx.tableIdx].getSymbolType(idx.symbolIdx)) {
			argName = symbolTables[idx.tableIdx].getName() + "_var_" + argName;//函数名_var_变量名
		}
	}
	return argName;
}
/*
	打印四元式
*/
void SemanticAnalyzer::printQuads() {
	for (auto iter = quads.begin(); iter != quads.end(); iter++) {
		quadsPrinter << setiosflags(ios::right);
		quadsPrinter << setw(3) << iter->first << "  (" << (iter->second).op << ", " << (iter->second).arg1 << ", " << (iter->second).arg2 << ", " << (iter->second).result << ")" << endl;
	}
} 

/*
	在四元式表中添加四元式
*/
void SemanticAnalyzer::setQuad(const Quad& quad) {
	quads[quad.idx] = quad;
}

/*
	回填四元式 此时每次回填值只有一个 但是回填四元式不一定只有一个
	这些待回填四元式回填同一个值
*/
void SemanticAnalyzer::backpatch(const vector<int>& backpatchQuad, int backpatchValue) {
	for (auto iter = backpatchQuad.begin(); iter != backpatchQuad.end(); iter++) {
		quads[*iter].result = to_string(backpatchValue);
	}
}
/*
	创建一张xx符号表(函数符号表)
*/
void SemanticAnalyzer::createSymbolTable(TableType type, const string& name) {
	symbolTables.push_back(SymbolTable(type, name));//创建xx符号表
	int index = symbolTables.size() - 1;//当前xx符号表的索引
	displayTable.push_back(index);//将当前表索引加入display表 表示现在分析该表
}
/*
	语法分析 规约时执行语义检查和属性计算 返回布尔值
	每个产生式应当都有语义动作  为了方便理解和分析 从最后一个产生式进行分析
*/
bool SemanticAnalyzer::semanticCheck(vector<GrammarSymbol>& grammarSymbolAttrStack, const Production& prod) {
	GrammarSymbol symbol;
	if (prod.left == "ConstValue") {//ConstValue->const_int|const_float|const_char
		//此时要为产生式左边的非终结符创建一个属性
		GrammarSymbol const_int = grammarSymbolAttrStack.back();//取文法符号属性栈栈顶元素

		symbol.name = prod.left;
		symbol.value = const_int.value;
	}
	else if (prod.left == "Factor" && prod.right[0] == "ConstValue") {//ConstValuector->ConstValue
		//此处需要创建临时变量 并将其加入临时变量符号表
		//此时要为产生式左边的非终结符创建一个属性
		GrammarSymbol constValue = grammarSymbolAttrStack.back();//取文法符号属性栈栈顶元素

		symbol.name = prod.left;
		//symbol.value = T_x=varName
		//申请临时变量
		int symbolIdx = symbolTables[1].setSymbol(constValue.value);//在临时变量表中加入临时变量并返回索引
		string varName = symbolTables[1].getVarName(symbolIdx);//得到临时变量名
		symbol.value = varName;
		symbol.idx.tableIdx = 1;
		symbol.idx.symbolIdx = symbolIdx;

		//此处需要生成一个四元式
		setQuad(Quad{ getNextQuad(),":=",constValue.value,"-",varName });

		//目标代码 常量赋值指令(addi $ti,$0,imm)
		targetCodeGenerator.addiImmToReg(constValue.value, symbol.idx);
	}
	else if (prod.left == "Factor" && prod.right[0] == "(") {//Factor->( Expression ) 
		//Expression的value属性、Index属性赋给Factor
		//此时要为产生式左边的非终结符创建一个属性
		GrammarSymbol expression = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 1];//Expression

		symbol.name = prod.left;
		symbol.value = symbolTables[expression.idx.tableIdx].getVarName(expression.idx.symbolIdx);
		symbol.idx = expression.idx;
	}
	else if (prod.left == "Factor" && prod.right[0] == "id") {//Factor->id FTYPE
		//碰到标识符id则需要进行检查 看它是否定义
		//还要分析FTYPE->CallFunction|$ 看ftype是由哪个产生式规约来的
		GrammarSymbol ftype = grammarSymbolAttrStack.back();
		GrammarSymbol& id = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 1];//此处必须是引用 需要修改值

		symbol.name = prod.left;

		//从当前函数定义域开始检查 看是否定义过该变量
		int idx = -1;
		for (auto iter = displayTable.rbegin(); iter != displayTable.rend(); ++iter) {//display表是xx符号表的索引
			SymbolTable st = symbolTables[*iter];//得到xx符号表
			idx = st.getSymbolIdx(id.value);//通过文法符号值在符号表中查找其索引
			if (idx != -1) {//找到了
				id.idx.tableIdx = *iter;
				id.idx.symbolIdx = idx;
				break;
			}
		}
		if (idx == -1) {
			cerr << "Semantic Analysis ERROR!" << endl;
			cerr << "Identifier " << id.value << " was not defined! Please check if you have defined it!" << endl;
			return false;//报错
		}

		//标识符已经定义,检查是函数名还是变量名
		if (ftype.value == "") {//变量名  FTYPE->$ => Factor->id
			//检查id是否是临时变量 通过所在表索引找到该符号类型
			if (symbolTables[id.idx.tableIdx].getSymbolType(id.idx.symbolIdx) == SymbolType::TEMP_VAR) {//是临时变量
				symbol.value = symbolTables[1].getVarName(id.idx.symbolIdx);//在临时变量表中找到其名字
				symbol.idx = id.idx;
			}
			else {//不是临时变量
				//申请临时变量
				int symbolIdx = symbolTables[1].setSymbol(id.value);//在临时变量表中加入临时变量并返回索引
				string varName = symbolTables[1].getVarName(symbolIdx);//得到临时变量名
				symbol.value = varName;
				symbol.idx = { 1,symbolIdx };

				//此处需要生成一个四元式 varName = getArgName()
				setQuad(Quad{ getNextQuad(),":=",getArgName(id.idx),"-",varName });

				//目标代码 赋值指令(add $t1,$t2,$0) 
				//$t1为申请的临时变量 $2为通过索引查找符号表 符号 找到的寄存器
				//(寄存器可能已经有值了 若没值符号的regIdx属性=-1 需要在内存中开辟空间 并将值赋给临时变量)
				string rs = targetCodeGenerator.getOperand(symbol.idx);
				string rt = targetCodeGenerator.getOperand(id.idx);

				targetCodeGenerator.setInstruction({ "add",rs,rt,"$0" });
			}
		}
		else {//FTYPE->CallFunction  => Factor->id CallFunction //函数名
			//申请临时变量 用该临时变量存储函数返回值
			int symbolIdx = symbolTables[1].setSymbol(id.value);
			string varName = symbolTables[1].getVarName(symbolIdx);
			//得到当前函数返回值
			int tableIdx = symbolTables[id.idx.tableIdx].getSymbol(id.idx.symbolIdx).funcTableIdx;//函数符号表的索引
			Index idx = { tableIdx, 0 };//返回值存放在函数符号表的0索引

			//此处需要生成一个四元式
			//将函数返回值存在临时变量
			setQuad(Quad{ getNextQuad(),":=",getArgName(idx),"-",varName });

			symbol.value = "";
			symbol.idx = { 1,symbolIdx };//存到临时变量表

			//目标代码 
			//函数调用前需要清理寄存器
			targetCodeGenerator.clearRegs();
			//生成调用函数指令
			targetCodeGenerator.setInstruction({"jal","","",id.value});
			//从函数调用返回 调整fp sp 指针
			//sp指向fp
			targetCodeGenerator.setInstruction({ "add","$sp","$fp","$0" });
			//fp 指向老fp 由于在创建活动记录时 将老$fp的值存在地址为$sp的内存中  在将新的fp指向sp
			//故 ($fp)代表的地址就是活动记录创建时$sp的值 那么老的fp就被存在($fp)中
			targetCodeGenerator.setInstruction({ "lw","$fp","($fp)","" });
			//$sp指针需要回退 
			int paraNum = symbolTables[0].getSymbol(id.idx.symbolIdx).formalParaNum;
			targetCodeGenerator.incStackPointer(-1 * paraNum);

			//此处需要注意 是函数调用 需要将函数返回值存在$v0(or $v1)
			string rs = targetCodeGenerator.getOperand(symbol.idx);

			targetCodeGenerator.setInstruction({ "add",rs,"$v0","$0" });
		}
	}
	else if (prod.left == "FactorLoop" && prod.right[0] == "FactorLoop") {//FactorLoop->FactorLoop Factor *|FactorLoop Factor /
		//此处需要为Factor创建临时变量存起来
		//此时要为产生式左边的非终结符创建一个属性
		GrammarSymbol factor = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 1];
		GrammarSymbol factorLoop = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 2];

		symbol.name = prod.left;

		if (factorLoop.value == "") {//FactorLoop->$ ==> FactorLoop->Factor *
			//此处不需要申请临时变量 只需将 属性栈顶的* /赋给value即可
			//Factor的值已经是临时变量了 只需将Factor里面的临时变量的 表和符号索引赋给idx即可
			symbol.value = grammarSymbolAttrStack.back().name;//* /
			symbol.idx = factor.idx;
		}
		else {//FactorLoop->FactorLoop Factor *|FactorLoop Factor /
			//FactorLoop = Factor * 存的值是临时变量
			//此时需要计算 FactorLoop 里面的值1 和 Factor里面的值2  乘除结果
			string arg1 = getArgName(factorLoop.idx);
			string arg2 = getArgName(factor.idx);

			string rs = targetCodeGenerator.getOperand(factorLoop.idx);
			string rt = targetCodeGenerator.getOperand(factor.idx);

			int  nextQuad = getNextQuad();
			if (factorLoop.value == "*") {
				//生成计算四元式
				setQuad(Quad{ nextQuad,"*",arg1,arg2,arg1 });//arg1 = arg1*arg2
				//目标代码 乘法指令(mult rs,rt mflo rs)
				targetCodeGenerator.setInstruction({ "mult",rs, rt,"" });
				targetCodeGenerator.setInstruction({ "mflo",rs,"","" });//取低32位
			}
			else if (factorLoop.value == "/") {
				setQuad(Quad{ nextQuad,"/",arg1,arg2,arg1 });//arg1 = arg1/arg2
				//目标代码 除法指令(div rs,rt mflo rs) 
				targetCodeGenerator.setInstruction({ "div",rs,rt,"" });
				targetCodeGenerator.setInstruction({ "mflo",rs,"","" });//取低32位
			}
			symbol.value = factorLoop.value;// * /
			symbol.idx = factorLoop.idx;
		}
	}
	else if (prod.left == "Item") {//Item->FactorLoop Factor
		GrammarSymbol factorLoop = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 1];
		GrammarSymbol factor = grammarSymbolAttrStack.back();

		symbol.name = prod.left;
		//判断FactorLoop是否为空
		if (factorLoop.value == "") {//FactorLoop->$ =>Item -> Factor
			//此处无需申请临时变量 直接将Factor的临时变量值传给Item即可(临时变量传值只需传表和符号索引即可)
			symbol.value = "";
			symbol.idx = factor.idx;
		}
		else {//FactorLoop不空
			//FactorLoop中的 Factor 临时变量值 
			//Factor 中的 临时变量值
			//根据FactorLoop.value = */ 进行运算 最后还要存在arg1中
			string arg1 = getArgName(factorLoop.idx);
			string arg2 = getArgName(factor.idx);

			string rs = targetCodeGenerator.getOperand(factorLoop.idx);
			string rt = targetCodeGenerator.getOperand(factor.idx);

			int  nextQuad = getNextQuad();
			if (factorLoop.value == "*") {
				//生成计算四元式
				setQuad(Quad{ nextQuad,"*",arg1,arg2,arg1 });//arg1 = arg1*arg2
				//目标代码 乘法指令(mult rs,rt mflo rs)
				targetCodeGenerator.setInstruction({ "mult",rs, rt,"" });
				targetCodeGenerator.setInstruction({ "mflo",rs,"","" });//取低32位
			}
			else if (factorLoop.value == "/") {
				setQuad(Quad{ nextQuad,"/",arg1,arg2,arg1 });//arg1 = arg1/arg2
				//目标代码 除法指令(div rs,rt mflo rs) 
				targetCodeGenerator.setInstruction({ "div",rs,rt,"" });
				targetCodeGenerator.setInstruction({ "mflo",rs,"","" });//取低32位

			}
			//将计算值存在Item中
			symbol.value = factorLoop.value;// * /
			symbol.idx = factorLoop.idx;
		}
	}
	else if (prod.left == "ItemLoop" && prod.right[0] == "ItemLoop") {//ItemLoop->ItemLoop Item +|ItemLoop Item -
		GrammarSymbol itemLoop = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 2];
		GrammarSymbol item = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 1];

		symbol.name = prod.left;
		//判定ItemLoop是否为空
		if (itemLoop.value == "") {//ItemLoop->$  ItemLoop->Item
			symbol.value = grammarSymbolAttrStack.back().name;//+ -
			symbol.idx = item.idx;
		}
		else {//ItemLoop->ItemLoop Item + Item |ItemLoop Item - Item
			string arg1 = getArgName(itemLoop.idx);
			string arg2 = getArgName(item.idx);

			string rs = targetCodeGenerator.getOperand(itemLoop.idx);
			string rt = targetCodeGenerator.getOperand(item.idx);

			int  nextQuad = getNextQuad();
			if (itemLoop.value == "+") {
				//生成计算四元式
				setQuad(Quad{ nextQuad,"+",arg1,arg2,arg1 });//arg1 = arg1+arg2
				//目标代码 加法指令(add rs,rt,rd)(rs = rt+rd)
				targetCodeGenerator.setInstruction({ "add",rs,rs,rt });
			}
			else if (itemLoop.value == "-") {
				//生成计算四元式
				setQuad(Quad{ nextQuad,"-",arg1,arg2,arg1 });//arg1 = arg1-arg2
				//目标代码 减法指令(sub rs,rt,rd)(rs = rt-rd)
				targetCodeGenerator.setInstruction({ "sub",rs,rs,rt });

			}
			symbol.value = grammarSymbolAttrStack.back().value;// + -
			symbol.idx = itemLoop.idx;
		}
	}
	else if (prod.left == "AddExpression") {//AddExpression->ItemLoop Item
		GrammarSymbol itemLoop = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 1];
		GrammarSymbol item = grammarSymbolAttrStack.back();

		symbol.name = prod.left;
		//判断ItemLoop是否为空
		if (itemLoop.value == "") {//AddExpression->Item
			//直接传递临时变量即可
			//此处无需申请临时变量 直接将Item的临时变量值传给AddExpression即可(临时变量传值只需传表和符号索引)
			symbol.value = "";
			symbol.idx = item.idx;
		}
		else {//ItemLoop不空
		//ItemLoop中的 Item 临时变量值 
		//Item 中的 临时变量值
		//根据ItemLoop.value = +- 进行运算 最后还要存在arg1中
			string arg1 = getArgName(itemLoop.idx);
			string arg2 = getArgName(item.idx);

			string rs = targetCodeGenerator.getOperand(itemLoop.idx);
			string rt = targetCodeGenerator.getOperand(item.idx);

			int  nextQuad = getNextQuad();
			if (itemLoop.value == "+") {
				//生成计算四元式
				setQuad(Quad{ nextQuad,"+",arg1,arg2,arg1 });//arg1 = arg1+arg2
				//目标代码 加法指令(add rs,rt,rd)(rs = rt+rd)
				targetCodeGenerator.setInstruction({ "add",rs,rs,rt });
			}
			else if (itemLoop.value == "-") {
				setQuad(Quad{ nextQuad ,"-",arg1,arg2,arg1 });//arg1 = arg1-arg2
				//目标代码 减法指令(sub rs,rt,rd)(rs = rt-rd)
				targetCodeGenerator.setInstruction({ "sub",rs,rs,rt });
			}
			symbol.value = itemLoop.value;// + -
			symbol.idx = itemLoop.idx;
		}
	}
	else if (prod.left == "AddExpressionLoop" && prod.right[0] == "AddExpressionLoop") {//AddExpressionLoop->AddExpressionLoop AddExpression Relop
		GrammarSymbol addExpression = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 1];
		GrammarSymbol addExpressionLoop = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 2];

		symbol.name = prod.left;

		if (addExpressionLoop.value == "") {//AddExpressionLoop->$ ==>AddExpressionLoop->AddExpression Relop
			//无需创建临时变量 直接将其AddExpression的值进行传递
			symbol.value = grammarSymbolAttrStack.back().value;//> < >= <= ....
			symbol.idx = addExpression.idx;
		}
		else {//AddExpressionLoop->AddExpressionLoop AddExpression Relop
			//此处需要生成四元式
			/*
			四元式的生成规则如下：
				nextquad (jop,X1,X2, 0); trueList
				nextquad+1 (j  ,- ,- , 0); falseList
			此处是待回填的四元式
			*/
			string arg1 = getArgName(addExpressionLoop.idx);
			string arg2 = getArgName(addExpression.idx);

			string rs = targetCodeGenerator.getOperand(addExpressionLoop.idx);
			string rt = targetCodeGenerator.getOperand(addExpression.idx);

			//四元式
			int nextQuadTrue = getNextQuad();
			symbol.trueList.push_back(nextQuadTrue);
			setQuad(Quad{ nextQuadTrue,"j" + addExpressionLoop.value,arg1,arg2,"0" });//真出口

			//目标代码 比较跳转指令 即（cmp rs,rt,branchLabel）
			int inst;
			if (addExpressionLoop.value == ">") {//branchLabel 先空着  之后需要回填
				inst = targetCodeGenerator.setInstruction({ "bgt",rs,rt, "" });
			}
			else if (addExpressionLoop.value == ">=") {
				inst = targetCodeGenerator.setInstruction({ "bge",rs,rt,"" });
			}
			else if (addExpressionLoop.value == "<") {
				inst = targetCodeGenerator.setInstruction({ "blt",rs,rt,"" });
			}
			else if (addExpressionLoop.value == "<=") {
				inst = targetCodeGenerator.setInstruction({ "ble",rs,rt,"" });
			}
			else if (addExpressionLoop.value == "==") {
				inst = targetCodeGenerator.setInstruction({ "beq",rs,rt,"" });
			}
			else if (addExpressionLoop.value == "!=") {
				inst = targetCodeGenerator.setInstruction({ "bne",rs,rt,"" });
			}
			//真出口指令  待回填
			symbol.trueLabel.push_back(inst);


			int nextQuadFalse = getNextQuad();
			symbol.falseList.push_back(nextQuadFalse);
			setQuad(Quad{ nextQuadFalse,"j","-","-","0" });//假出口

			//目标代码 假出口的跳转指令 待回填
			int inst_ = targetCodeGenerator.setInstruction({ "j","","","" });
			symbol.falseLabel.push_back(inst_);

			symbol.value = addExpressionLoop.value;//> < >= <= ....
			symbol.idx = addExpressionLoop.idx;
		}
	}
	else if (prod.left == "Relop") {//Relop-><|>|>=|<=|!=|==
		GrammarSymbol op = grammarSymbolAttrStack.back();

		symbol.name = prod.left;
		symbol.value = op.name;//<|>|>=|<=|!=|==
	}
	else if (prod.left == "Expression") {//Expression->AddExpressionLoop AddExpression 其实同前面的AddExpressLoop产生式
		GrammarSymbol addExpression = grammarSymbolAttrStack.back();
		GrammarSymbol addExpressionLoop = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 1];

		symbol.name = prod.left;
		//判断AddExpressionLoop是否为空
		if (addExpressionLoop.value == "") {//AddExpressionLoop -> $ ==> Expression->AddExpression
			//无需创建临时变量 直接传递属性即可
			symbol.value = addExpression.value;
			symbol.idx = addExpression.idx;
		}
		else {//不空
			//此处需要生成四元式
			/*
			四元式的生成规则如下：
				nextquad (jop,X1,X2, 0); trueList
				nextquad+1 (j  ,- ,- , 0); falseList
			此处是待回填的四元式
			*/
			string arg1 = getArgName(addExpressionLoop.idx);
			string arg2 = getArgName(addExpression.idx);

			string rs = targetCodeGenerator.getOperand(addExpressionLoop.idx);
			string rt = targetCodeGenerator.getOperand(addExpression.idx);

			int nextQuadTrue = getNextQuad();
			symbol.trueList.push_back(nextQuadTrue);
			setQuad(Quad{ nextQuadTrue,"j" + addExpressionLoop.value,arg1,arg2,"0" });//真出口

			//目标代码 比较跳转指令 即（cmp rs,rt,branchLabel）
			int inst;
			if (addExpressionLoop.value == ">") {//branchLabel 先空着  之后需要回填
				inst = targetCodeGenerator.setInstruction({ "bgt",rs,rt, "" });
			}
			else if (addExpressionLoop.value == ">=") {
				inst = targetCodeGenerator.setInstruction({ "bge",rs,rt,"" });
			}
			else if (addExpressionLoop.value == "<") {
				inst = targetCodeGenerator.setInstruction({ "blt",rs,rt,"" });
			}
			else if (addExpressionLoop.value == "<=") {
				inst = targetCodeGenerator.setInstruction({ "ble",rs,rt,"" });
			}
			else if (addExpressionLoop.value == "==") {
				inst = targetCodeGenerator.setInstruction({ "beq",rs,rt,"" });
			}
			else if (addExpressionLoop.value == "!=") {
				inst = targetCodeGenerator.setInstruction({ "bne",rs,rt,"" });
			}
			//真出口
			symbol.trueLabel.push_back(inst);

			int nextQuadFalse = getNextQuad();
			symbol.falseList.push_back(nextQuadFalse);
			setQuad(Quad{ nextQuadFalse,"j","-","-","0" });//假出口

			//目标代码 假出口跳转指令
			int inst_ = targetCodeGenerator.setInstruction({"j","","",""});
			symbol.falseLabel.push_back(inst_);

			symbol.value = addExpressionLoop.value;//> < >= <= ....
			symbol.idx = addExpressionLoop.idx;
		}
	}
	else if (prod.left == "If_M1"){// If_M1->$
		symbol.name = prod.left;
		symbol.value = "";
		symbol.quad = getNextQuad();//M.quad = nextquad;
		this->nextQuad--;//此时nextQuad要-1 否则下一个四元式标号不对

		//进入if前 清空寄存器
		targetCodeGenerator.clearRegs();

		this->label++;//进入if语句 标号+1
		labelStack.push_back(label);
		int inst = targetCodeGenerator.setInstruction({ "__if"+to_string(labelStack.back()),":","","" }); //返回这条指令的序号
		//__if0
		symbol.inst = inst;
	}
	else if (prod.left == "If_M2") {// If_M2->$ 
		symbol.name = prod.left;
		symbol.value = "";
		symbol.quad = getNextQuad();//M.quad = nextquad;
		this->nextQuad--;//此时nextQuad要-1 否则下一个四元式标号不对
		
		//进入else前清空寄存器
		targetCodeGenerator.clearRegs();

		//else 分支 label不需要+1 去当前if语句块标签即可
		int inst = targetCodeGenerator.setInstruction({ "__else" + to_string(labelStack.back()),":","","" }); //返回这条指令的序号
		//__else0
		symbol.inst = inst;//以后回填
	}
	else if (prod.left == "If_N") {// If_N->$
		symbol.name = prod.left;
		symbol.value = "";
		int nextQuad = getNextQuad();
		symbol.nextList.push_back(nextQuad);
		//生成一个四元式
		setQuad(Quad{ nextQuad,"j","-","-","-" });//待回填

		//退出if else 语句 清空寄存器
		targetCodeGenerator.clearRegs();

		//目标代码 防止执行else
		int inst = targetCodeGenerator.setInstruction({ "j","","","" });
		
		symbol.nextLabel.push_back(inst);
	}
	
	else if (prod.left == "ElseSentenceBlock" && prod.right[0] == "If_N") {//ElseSentenceBlock->$|If_N else If_M2 SentenceBlock
		GrammarSymbol sentenceBlock = grammarSymbolAttrStack.back();
		GrammarSymbol if_n = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 3];
		GrammarSymbol if_m2 = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 1];

		symbol.name = prod.left;
		symbol.inst = if_m2.inst;
		symbol.value = to_string(sentenceBlock.quad);//sentenceBlock.quad必不空 规约时回填Ifsentence.nextlist
		symbol.quad = if_m2.quad;//将If_M2的值存在elseSentenceBlock的quad里面  在if...else...语句中要回填If_M2.quad
		//merge(N.nextlist,S2.nextlist)
		symbol.nextList.insert(symbol.nextList.end(), sentenceBlock.nextList.begin(), sentenceBlock.nextList.end());
		symbol.nextList.insert(symbol.nextList.end(), if_n.nextList.begin(), if_n.nextList.end());
		//回填指令
		symbol.nextLabel.insert(symbol.nextLabel.end(), sentenceBlock.nextLabel.begin(), sentenceBlock.nextLabel.end());
		symbol.nextLabel.insert(symbol.nextLabel.end(), if_n.nextLabel.begin(), if_n.nextLabel.end());

		//退出else前 清空寄存器
		targetCodeGenerator.clearRegs();

	}
	else if (prod.left == "IfSentence") {//IfSentence->if ( Expression ) If_M1 SentenceBlock ElseSentenceBlock
		GrammarSymbol elseSentenceBlock = grammarSymbolAttrStack.back();
		GrammarSymbol sentenceBlock = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 1];
		GrammarSymbol if_m1 = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 2];
		GrammarSymbol expression = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 4];

		symbol.name = prod.left;
		symbol.value = "";
		if (elseSentenceBlock.value == "") {//ElseSentenceBlock->$  ==>IfSentence->if ( Expression ) If_M1 SentenceBlock
			//merge(E.falselist,S1.nextlist)
			symbol.nextList.insert(symbol.nextList.end(), expression.falseList.begin(), expression.falseList.end());
			symbol.nextList.insert(symbol.nextList.end(), sentenceBlock.nextList.begin(), sentenceBlock.nextList.end());
			//backpatch(E.truelist,M1.quad)
			backpatch(expression.trueList, if_m1.quad);
			//if ... then ...需要用sentenceBlock.quad 回填symbol.nextlist 
			backpatch(symbol.nextList, sentenceBlock.quad);
			////////////////////////////////////////////////////////////////////////////////////////////////////////
			
			//退出只有if的语句前 需要清空寄存器
			targetCodeGenerator.clearRegs();

			//目标代码回填
			symbol.nextLabel.insert(symbol.nextLabel.end(), expression.falseLabel.begin(), expression.falseLabel.end());
			symbol.nextLabel.insert(symbol.nextLabel.end(), sentenceBlock.falseLabel.begin(), sentenceBlock.falseLabel.end());
			//回填指令
			targetCodeGenerator.backpatch_(expression.trueLabel,if_m1.inst);
			int inst = targetCodeGenerator.setInstruction({ "__end_if"+to_string(labelStack.back()),":","","" });//此处只有if语句 结束标签
			targetCodeGenerator.backpatch_(symbol.nextLabel, inst);
			//if语句结束之后labelStack需要弹栈
			labelStack.pop_back();
		}
		else {//IfSentence->if ( Expression ) If_M1 SentenceBlock1 If_N else If_M2 SentenceBlock2
			//merge(S1.nextlist,N.nextlist,S2.nextlist)
			symbol.nextList.insert(symbol.nextList.end(), sentenceBlock.nextList.begin(), sentenceBlock.nextList.end());
			symbol.nextList.insert(symbol.nextList.end(), elseSentenceBlock.nextList.begin(), elseSentenceBlock.nextList.end());
			//回填
			//backpatch(E.truelist,M1.quad)
			//backpatch(E.falselist,M2.quad)
			backpatch(expression.trueList, if_m1.quad);
			backpatch(expression.falseList, elseSentenceBlock.quad);
			//if ... then ...需要用elseSentenceBlock.value 回填symbol.nextlist
			backpatch(symbol.nextList, stoi(elseSentenceBlock.value));
			////////////////////////////////////////////////////////////////////////////////////////////////////////
			//目标代码回填
			symbol.nextLabel.insert(symbol.nextLabel.end(), sentenceBlock.nextLabel.begin(), sentenceBlock.nextLabel.end());
			symbol.nextLabel.insert(symbol.nextLabel.end(), elseSentenceBlock.nextLabel.begin(), elseSentenceBlock.nextLabel.end());
			
			targetCodeGenerator.backpatch_(expression.trueLabel, if_m1.inst);
			targetCodeGenerator.backpatch_(expression.falseLabel, elseSentenceBlock.inst);//if_m2.inst回填
			//回填nextlabel
			int inst = targetCodeGenerator.setInstruction({"__end_if_else"+to_string(labelStack.back()),":","",""});
			targetCodeGenerator.backpatch_(symbol.nextLabel,inst);

			//if else 语句结束 弹栈
			labelStack.pop_back();
;		}
	}
	else if (prod.left == "While_M1") {// While_M1 -> $ 
		symbol.name = prod.left;
		symbol.value = "";
		symbol.quad = getNextQuad();//M.quad = nextquad;
		this->nextQuad--;//此时nextQuad要-1 否则下一个四元式标号不对

		//目标代码回填
		//进入循环前 清空寄存器
		targetCodeGenerator.clearRegs();

		//进入循环语句 标签+1
		this->label++;
		labelStack.push_back(label);
		int inst = targetCodeGenerator.setInstruction({"__while"+to_string(labelStack.back()),":","",""});
		symbol.inst = inst;
	}
	else if (prod.left == "While_M2") {// While_M2 -> $ 
		symbol.name = prod.left;
		symbol.value = "";
		symbol.quad = getNextQuad();//M.quad = nextquad;
		this->nextQuad--;//此时nextQuad要-1 否则下一个四元式标号不对
		
		//目标代码回填
		int inst = targetCodeGenerator.setInstruction({ "__while_begin"+to_string(labelStack.back()),":","","" });
		symbol.inst = inst;

	}
	else if (prod.left == "WhileSentence") {//WhileSentence->while While_M1 ( Expression ) While_M2 SentenceBlock
		GrammarSymbol sentenceBlock = grammarSymbolAttrStack.back();
		GrammarSymbol while_m2 = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 1];
		GrammarSymbol expression = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 3];
		GrammarSymbol while_m1 = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 5];

		symbol.name = prod.left;
		symbol.value = "";
		//回填
		//backpatch(E.truelist,M2.quad)
		//backpatch(S.nextlist,M1.quad)
		backpatch(expression.trueList, while_m2.quad);
		backpatch(sentenceBlock.nextList, while_m1.quad);
		//S.nextlist = E.falselist
		symbol.nextList = expression.falseList;

		//目标代码回填
		targetCodeGenerator.backpatch_(expression.trueLabel, while_m2.inst);
		targetCodeGenerator.backpatch_(sentenceBlock.nextLabel, while_m1.inst);//其实sentenceBlock的nextList  nextLabel都为空
		symbol.nextLabel = expression.falseLabel;


		//生成一个四元式
		//注意该四元式不在SentenceBlock中  它是控制循环的
		setQuad(Quad{ getNextQuad(),"j","-","-",to_string(while_m1.quad) });

		//结束本次循环前 清空寄存器
		targetCodeGenerator.clearRegs();

		//目标代码 控制循环
		targetCodeGenerator.setInstruction({ "j","","",targetCodeGenerator.getInst(while_m1.inst).op });//j __while


		//用SentenceBlock.quad + 1回填 WhileSentence.nextlist 表示退出循环
		//主要是多了一句(j,-,-,M1.quad) 它并不是在SentenceBlock中生成的四元式 要跳过这一句
		backpatch(symbol.nextList, sentenceBlock.quad + 1);

		int inst = targetCodeGenerator.setInstruction({"__while_end"+to_string(labelStack.back()),":","",""});
		//目标代码回填 while的nextlabel
		targetCodeGenerator.backpatch_(symbol.nextLabel, inst);

		//退出循环 labelStack 弹栈
		labelStack.pop_back();
		
	}
	else if (prod.left == "AssignSentence") {//	AssignSentence->id = Expression ;
		GrammarSymbol expression = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 1];
		GrammarSymbol id = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 3];

		symbol.name = prod.left;
		//碰到identifier需要检查是否定义过  (当前函数符号表和全局符号表)
		int symbolIdx = -1;
		int tableIdx = 0;
		for (auto iter = displayTable.rbegin(); iter != displayTable.rend(); ++iter) {
			SymbolTable st = symbolTables[*iter];//得到xx符号表
			symbolIdx = st.getSymbolIdx(id.value);//通过文法符号值在符号表中查找其索引
			if (symbolIdx != -1) {//找到了
				tableIdx = *iter;
				break;
			}
		}
		if (symbolIdx == -1) {//没定义过 报错
			cerr << "Semantic Analysis ERROR!" << endl;
			cerr << "Identifier " << id.value << " was not defined! Please check if you have defined it!" << endl;
			return false;//报错
		}
		//标识符已经定义
		string arg1 = getArgName(expression.idx);
		string result = getArgName(Index{ tableIdx,symbolIdx });
		//生成一个四元式
		setQuad(Quad{ getNextQuad(),":=",arg1,"-",result });

		//赋值指令 add rs,rt,$0 //rs = rt;
		string rs = targetCodeGenerator.getOperand(Index{ tableIdx,symbolIdx });
		string rt = targetCodeGenerator.getOperand(expression.idx);
		targetCodeGenerator.setInstruction({ "add",rs,rt,"$0" });


		symbol.value = "";//空
	}
	else if (prod.left == "ReturnExpression" && prod.right[0] == "$") {//ReturnExpression->$
		symbol.name = prod.left;
		symbol.value = "";//返回值类型为空

		//ReturnSentence->return ReturnExpression($) ;
		//此时栈顶是return
		//返回类型是void 
		//判断当前函数符号表中0索引处 返回值类型
		if (VarType::VOID != symbolTables[displayTable.back()].getVarType(0)) {
			cerr << "Semantic Analysis ERROR!" << endl;
			cerr << "The return value type of the function does not match the declared type! Please Check it!" << endl;
			return false;
		}
	}
	else if (prod.left == "ReturnExpression" && prod.right[0] == "Expression") {//ReturnExpression->Expression
		//ReturnSentence->return ReturnExpression ;
		//此时返回值不是void
		if (VarType::INT != symbolTables[displayTable.back()].getVarType(0)) {
			cerr << "Semantic Analysis ERROR!" << endl;
			cerr << "The return value type of the function does not match the declared type! Please Check it!" << endl;
			return false;
		}
		GrammarSymbol expression = grammarSymbolAttrStack.back();

		symbol.name = prod.left;
		symbol.value = "int";//返回值是int
		//此处Expression代表的临时变量索引直接传给ReturnExpression
		symbol.idx = expression.idx;

		//函数 返回值 存在$v0 中
		string rt = targetCodeGenerator.getOperand(expression.idx);
		targetCodeGenerator.setInstruction({"add","$v0",rt,"$0"});
		// 函数返回地址存在 $ra
		targetCodeGenerator.setInstruction({"lw","$ra","4($fp)",""});//fp + 4 存放返回地址
	}
	else if (prod.left == "ReturnSentence") {//ReturnSentence->return ReturnExpression ;
		GrammarSymbol returnExpression = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 1];

		symbol.name = prod.left;

		if (returnExpression.value == "") {//返回void类型值
			symbol.value = "";
		}
		else {//返回int 类型值
			symbol.value = "int";
			//将返回值放在函数符号表的0索引
			symbol.idx = { Index{displayTable.back(),0} };
			string arg1 = getArgName(returnExpression.idx);//ReturnExpression存储的临时变量
			string result = getArgName(symbol.idx);

			//生成一个返回值赋值四元式
			setQuad(Quad{ getNextQuad(),":=",arg1,"-",result });
		}
		//生成一个返回值四元式 此处是表示退出该函数定义
		//(return,-,-,function)
		setQuad(Quad{ getNextQuad(),"return","-","-",symbolTables[displayTable.back()].getName() });
	}
	else if (prod.left == "InternalVariableStmt") {//InternalVariableStmt->int id
		symbol.name = prod.left;

		GrammarSymbol id = grammarSymbolAttrStack.back();
		//碰到identifier就要看是都在函数中定义过
		//在当前函数符号表中(displayTable.back())查找该标识符  看是否定义过
		if (symbolTables[displayTable.back()].getSymbolIdx(id.value) != -1) {//找到 说明该标识符已经定义 重复定义 
			cerr << "Semantic Analysis ERROR!" << endl;
			cerr << "Identifier " << id.value << " has been defined!" << endl;
			return false;//报错
		}
		//在全局符号表中查找该标识符 看是否定义过 是否与函数名冲突
		int symbolIdx1 = symbolTables[0].getSymbolIdx(id.value);
		if (-1 != symbolIdx1 && symbolTables[0].getSymbolType(symbolIdx1) == SymbolType::FUNCTION) {//是否被定义为函数
			cerr << "Semantic Analysis ERROR!" << endl;
			cerr << "Identifier " << id.value << " has been defined as a FUNCTION!" << endl;
			return false;//报错
		}
		//没定义过 也不与函数名冲突 可以定义为变量
		Symbol var;
		var.type = SymbolType::VAR;
		var.varName = id.value;
		var.varValue = "";//空 还未赋值
		var.varType = VarType::INT;
		//将该变量加入当前函数符号表
		int  symbolIdx2 = symbolTables[displayTable.back()].setSymbol(var); //返回变量在函数符号表中索引

		symbol.value = id.value;//变量名x
		symbol.idx = { displayTable.back(), symbolIdx2 };

		targetCodeGenerator.incStackPointer(1);//sp = sp + 4; 为局部变量申请空间
	}
	else if (prod.left == "SentenceBlock") {//SentenceBlock->SB_M { InternalStmt SentenceString }
		//用于回填 更大语法单位的nextlist
		symbol.name = prod.left;
		symbol.value = "sb";//随意
		symbol.quad = getNextQuad();//回填
		this->nextQuad--;//此处必须-1

		////此处还需要生成一个标签 表示结束
		//int inst = targetCodeGenerator.setInstruction({ "__end",":","","" });
		//symbol.inst = inst;//回填
	}
	else if (prod.left == "Param") {//Param->int id  形参
		GrammarSymbol id = grammarSymbolAttrStack.back();

		symbol.name = prod.left;
		//碰到identifier就要看是否定义过
		//在当前函数符号表中(displayTable.back())查找该标识符  看是否定义过
		if (symbolTables[displayTable.back()].getSymbolIdx(id.value) != -1) {//找到 说明该标识符已经定义 重复定义 
			cerr << "Semantic Analysis ERROR!" << endl;
			cerr << "Identifier " << id.value << " has been defined!" << endl;
			return false;//报错
		}
		//在全局符号表中查找该标识符 看是否定义过 是否与函数名冲突
		int symbolIdx1 = symbolTables[0].getSymbolIdx(id.value);
		if (-1 != symbolIdx1 && symbolTables[0].getSymbolType(symbolIdx1) == SymbolType::FUNCTION) {//是否被定义为函数
			cerr << "Semantic Analysis ERROR!" << endl;
			cerr << "Identifier " << id.value << " has been defined as a FUNCTION!" << endl;
			return false;//报错
		}
		//没定义过 也不与函数名冲突 可以定义为变量
		Symbol var;
		var.type = SymbolType::VAR;
		var.varName = id.value;
		var.varValue = "";//空 还未赋值
		var.varType = VarType::INT;
		//将该变量加入当前函数符号表
		int  symbolIdx2 = symbolTables[displayTable.back()].setSymbol(var); //返回变量在函数符号表中索引

		symbol.value = id.value;//变量名x
		symbol.idx = { displayTable.back(), symbolIdx2 };

		//全局符号表中的当前函数符号 需要将形式参数个数加1
		int tableIdx = symbolTables[0].getSymbolIdx(symbolTables[displayTable.back()].getName());
		symbolTables[0].getSymbol(tableIdx).formalParaNum++;
	}
	else if (prod.left == "VariableStmt") {//VariableStmt->;
		symbol.name = prod.left;
		symbol.value = ";";
	}
	else if (prod.left == "FunctionBegin") { //FunctionBegin->$
		symbol.name = prod.left;
		symbol.value = "";
		//Stmt->int id FunctionBegin($) FunctionStmt|void id FunctionBegin($) FunctionStmt
		//此时的栈顶元素为id, FunctionStmt还没进栈
		GrammarSymbol id = grammarSymbolAttrStack.back();
		GrammarSymbol returnType = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 1];
		//在全局符号表中查找此函数是否定义
		if (-1 != symbolTables[0].getSymbolIdx(id.value)) {
			cerr << "Semantic Analysis ERROR!" << endl;
			cerr << "Identifier " << id.value << " has been defined as a FUNCTION!" << endl;
			return false;//报错
		}
		//没有定义 创建一个新函数符号表
		createSymbolTable(TableType::FUNCTION, id.value);
		//将该函数名加到全局符号表中
		Symbol func;
		func.type = SymbolType::FUNCTION;
		func.varName = id.value;
		func.varValue = "";
		func.varType = (returnType.value == "void") ? VarType::VOID : VarType::INT;
		func.funcTableIdx = symbolTables.size() - 1;//函数符号在全局符号表中的位置
		int symbolIdx = symbolTables[0].setSymbol(func);

		//创建函数返回值
		Symbol returnVar;
		returnVar.type = SymbolType::RETURN;
		returnVar.varName = symbolTables[displayTable.back()].getName() + "_return_value";
		returnVar.varValue = "";//返回值暂时为空
		returnVar.varType = (returnType.value == "void") ? VarType::VOID : VarType::INT;
		//将函数返回值加入到函数符号表的0索引（该变量是创建函数时第一个加进去的故索引为0）
		symbolTables[displayTable.back()].setSymbol(returnVar);

		int nextQuad = getNextQuad();
		//判断该标识符是否为main
		if (id.value == "main") {
			mainFuncLine = nextQuad;
		}

		//生成一个四元式 用于表示此时进入当前函数
		setQuad(Quad{ nextQuad, id.value,"-","-","-" });

		//目标代码 表示进入该函数
		targetCodeGenerator.setInstruction({ id.value,":","","" });
		//建立活动记录
		targetCodeGenerator.setActivityRecordHeader();
	}
	else if (prod.left == "FunctionEnd") {//FunctionEnd->$
		symbol.name = prod.left;
		symbol.value = "";

		//得到当前函数名称 用于判断是否是main函数
		string funcName = symbolTables[displayTable.back()].getName();

		//此处要特别注意 要在display表中退出本函数符号表
		displayTable.pop_back();

		targetCodeGenerator.clearRegs();//退出函数前 清空寄存器

		if (funcName == "main") {
			targetCodeGenerator.setInstruction({ "j","","","Exit" }); //go to Exit
			targetCodeGenerator.setInstruction({ "Exit",":","","" });
		}
		else {//普通函数  jr $ra
			targetCodeGenerator.setInstruction({ "jr","$ra","","" }); //go to $31寄存器存储的返回地址
		}
	}
	else if (prod.left == "StmtType" && prod.right[0] == "VariableStmt") {//StmtType->VariableStmt
		//此时直接将VariableStmt的属性传递即可
		symbol.name = prod.left;
		symbol.value = grammarSymbolAttrStack.back().value;//;
	}
	else if (prod.left == "Stmt") {//Stmt->int id StmtType
		GrammarSymbol id = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 1];
		GrammarSymbol stmtType = grammarSymbolAttrStack.back();

		symbol.name = prod.left;

		//判断StmtType
		if (stmtType.value == ";") {//定义变量
			//从当前函数定义域开始检查 看是否定义过该变量
			int idx = -1;
			for (auto iter = displayTable.rbegin(); iter != displayTable.rend(); ++iter) {//display表是xx符号表的索引
				SymbolTable st = symbolTables[*iter];//得到xx符号表
				idx = st.getSymbolIdx(id.value);//通过文法符号值在符号表中查找其索引
				if (idx != -1) {//找到了 说明已经定义过了
					cerr << "Semantic Analysis ERROR!" << endl;
					cerr << "Identifier " << id.value << " has been defined!" << endl;
					return false;//报错
				}
			}
			//没有定义过
			Symbol var;
			var.type = SymbolType::VAR;
			var.varName = id.value;
			var.varValue = "";//空,还未赋值
			var.varType = VarType::INT;
			symbolTables[displayTable.back()].setSymbol(var);

			symbol.value = id.value;

			targetCodeGenerator.incStackPointer(1);//sp = sp + 1; 定义一个局部变量 为其分配存储空间
		}
		//else {//定义函数 FunctionBegin->$时已经完成
		//}
	}
	else if (prod.left == "CallCheck") {//CallCheck->$
		//检查该函数是否定义
		//Factor->id ( CallCheck($) ActualParamList )   ActualParamList未进栈
		GrammarSymbol id = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 1];

		symbol.name = prod.left;
		//查找id是否定义
		int idx = -1;
		for (auto iter = displayTable.rbegin(); iter != displayTable.rend(); ++iter) {//display表是xx符号表的索引
			SymbolTable st = symbolTables[*iter];//得到xx符号表
			idx = st.getSymbolIdx(id.value);//通过文法符号值在符号表中查找其索引
			if (idx != -1) {//找到了
				break;
			}
		}
		if (idx == -1) {
			cerr << "Semantic Analysis ERROR!" << endl;
			cerr << "Identifier " << id.value << " was not defined! Please check if you have defined it!" << endl;
			return false;//报错
		}
		//定义了 看是否为函数调用
		if (SymbolType::FUNCTION != symbolTables[0].getSymbolType(idx)) {
			//不是函数调用 报错
			cerr << "Semantic Analysis ERROR!" << endl;
			cerr << "Identifier " << id.value << " is not a FUNCTION, it is not callable!" << endl;
			return false;
		}
		symbol.value = "";
		//把被调用函数符号的索引
		symbol.idx = { 0,idx };//{displayTable[*iter],idx}
	}
	else if (prod.left == "ExpressionLoop" && prod.right[0] == "$") {//ExpressionLoop->$
		//ActualParamList->ExpressionLoop($) Expression
		symbol.name = prod.left;
		symbol.value = "1";//至少1实参——Expression
	}
	else if (prod.left == "ExpressionLoop" && prod.right[0] == "ExpressionLoop") { //ExpressionLoop -> ExpressionLoop Expression ,
		//ActualParamList->ExpressionLoop Expression , 
		//此时栈顶为 ,
		GrammarSymbol expression = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 1];
		GrammarSymbol expressionLoop = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 2];
		GrammarSymbol callCheck = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 3];

		symbol.name = prod.left;

		//在CallCheck中找到存放的被调用函数符号
		Symbol& calledFunc = symbolTables[callCheck.idx.tableIdx].getSymbol(callCheck.idx.symbolIdx);
		int passedParamNum = stoi(expressionLoop.value);

		//此处无需进行参数个数判断 因为此时还在进行实参扫描
		//应该在规约为ActualList时进行判断 此时参数已经扫描完毕 

		/*int formalParamNum = calledFunc.formalParaNum;
		int passedParamNum = stoi(expressionLoop.value);
		if (formalParamNum < passedParamNum) {
			cerr << "Semantic Analysis ERROR!" << endl;
			cerr << "More parameters given!" << endl;
			cerr << "FUNCTION " << calledFunc.varName << " expected " << formalParamNum <<
				" parameter(s), but " << passedParamNum << " given! Please check it!" << endl;
			return false;
		}*/

		Index idx = { calledFunc.funcTableIdx,passedParamNum };//0是返回值
		string result = getArgName(idx, true);//返回值
		string arg1 = getArgName(expression.idx);
		//生成一个实参赋值四元式
		//func(a,b)
		//(:=,T1,-,func_param_a)
		//(:=,T2,-,func_param_b)
		//(call,-,-,func)
		setQuad(Quad{ getNextQuad(),":=",arg1,"-",result });

		//目标代码
		string rs = targetCodeGenerator.getOperand(expression.idx);
		targetCodeGenerator.setInstruction({ "sw",rs,"($sp)","" });//a = T1  (T1 = imm)  为临时变量分配存储空间
		targetCodeGenerator.incStackPointer(1);//sp  = sp + 4 

		//下一个参数
		passedParamNum++;
		//实参个数+1
		symbol.value = to_string(passedParamNum);
	}
	else if (prod.left == "ActualParamList" && prod.right[0] == "$") {//ActualParamList->$
		//CallFunction-> ( CallCheck ActualParamList($)
		//此时栈顶为CallCheck 
		GrammarSymbol callCheck = grammarSymbolAttrStack.back();

		symbol.name = prod.left;

		//在CallCheck中找到存放的被调用函数符号
		Symbol& calledFunc = symbolTables[callCheck.idx.tableIdx].getSymbol(callCheck.idx.symbolIdx);

		int formalParamNum = calledFunc.formalParaNum;//在被调用函数符号中得到形参个数
		if (0 != formalParamNum) {
			cerr << "Semantic Analysis ERROR!" << endl;
			cerr << "None parameters given!" << endl;
			cerr << "FUNCTION " << calledFunc.varName << " expected " << formalParamNum <<
				" parameter(s), but none given! Please check it!" << endl;
			return false;
		}

		symbol.value = "0";//0实参
	}
	else if (prod.left == "ActualParamList" && prod.right[0] == "ExpressionLoop") {
		//CallFunction->( CallCheck ExpressionLoop Expression
		GrammarSymbol expression = grammarSymbolAttrStack.back();
		GrammarSymbol expressionLoop = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 1];
		GrammarSymbol callCheck = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 2];

		symbol.name = prod.left;

		//在CallCheck中找到存放的被调用函数符号
		Symbol& calledFunc = symbolTables[callCheck.idx.tableIdx].getSymbol(callCheck.idx.symbolIdx);

		int formalParamNum = calledFunc.formalParaNum;//形参个数
		int passedParamNum = stoi(expressionLoop.value);//实际传递的实参个数

		if (formalParamNum < passedParamNum) {//传递参数过多
			cerr << "Semantic Analysis ERROR!" << endl;
			cerr << "More parameters given!" << endl;
			cerr << "FUNCTION " << calledFunc.varName << " expected " << formalParamNum <<
				" parameter(s), but " << passedParamNum << " given! Please check it!" << endl;
			return false;
		}

		//此处的passedParamNum为最后一个参数索引
		Index idx = { calledFunc.funcTableIdx,passedParamNum };
		string result = getArgName(idx, true);//返回值
		string arg1 = getArgName(expression.idx);
		//生成一个实参赋值四元式
		//func(a,b)
		//(:=,T1,-,func_param_a)
		//(:=,T2,-,func_param_b)
		//(call,-,-,func)

		setQuad(Quad{ getNextQuad(),":=",arg1,"-",result });

		//判断参数传递是否过少
		if (formalParamNum > passedParamNum) {
			cerr << "Semantic Analysis ERROR!" << endl;
			cerr << "Less parameters given!" << endl;
			cerr << "FUNCTION " << calledFunc.varName << " expected " << formalParamNum <<
				" parameter(s), but " << passedParamNum << " given! Please check it!" << endl;
			return false;
		}

		//目标代码
		string rs = targetCodeGenerator.getOperand(expression.idx);
		targetCodeGenerator.setInstruction({ "sw",rs,"($sp)","" });//func_param_x = T1  (T1 = a a = imm)  为临时变量分配存储空间 此处表示该临时变量是被调用函数实参
		targetCodeGenerator.incStackPointer(1);//sp  = sp + 4 

		symbol.value = to_string(passedParamNum);//传递的实参个数为正确的
	}
	else if (prod.left == "CallFunction") {//CallFunction->( CallCheck ActualParamList )
		//Factor->id ( CallCheck ActualParamList )
		GrammarSymbol id = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 4];
		GrammarSymbol callCheck = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 2];

		symbol.name = prod.left;
		symbol.value = "";
		symbol.idx = callCheck.idx;

		//生成一个函数调用四元式
		setQuad(Quad{ getNextQuad(),"call","-","-",id.value });
	}
	else if (prod.left == "FTYPE" && prod.right[0] == "CallFunction") {//FTYPE->CallFunction
		//传递属性即可
		GrammarSymbol callFunction = grammarSymbolAttrStack.back();

		symbol.name = prod.left;
		symbol.value = "CallFunction";//随意 但不为空  与ftype->$区别
		symbol.idx = callFunction.idx;
	}
	else if (prod.left == "Program") {
		//语义分析完毕
		//判断是否有main函数
		if (mainFuncLine == -1) {
			cerr << "Semantic Analysis ERROR!" << endl;
			cerr << "FUNCTION main was not defined! Please chack it!" << endl;
		}
		//生成第0个四元式
		setQuad(Quad{ 0,"j","-","-",to_string(mainFuncLine) });

		printQuads();
		targetCodeGenerator.printInst();

		symbol.name = prod.left;
		symbol.value = "";
	}
	else {
		symbol.name = prod.left;
		symbol.value = "";
	}

	//产生式右部长度
	int len = prod.right[0] != "$" ? prod.right.size() : 0;
	//规约之后 属性栈里面的被规约串出栈
	for (int i = 0; i < len; i++) {
		grammarSymbolAttrStack.pop_back();
	}
	//文法属性栈里面压入新文法符号、属性
	grammarSymbolAttrStack.push_back(symbol);

	return true;
}
