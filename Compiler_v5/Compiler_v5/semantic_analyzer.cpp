#include "semantic_analyzer.h"

/*
	��ʼ��  ����������
*/
SemanticAnalyzer::SemanticAnalyzer() {
	quadsPrinter.open("GeneratedData/Quads.txt");
	symbolTables.push_back(SymbolTable(TableType::GLOBAL, "global"));//ȫ�ַ��ű� [0]
	symbolTables.push_back(SymbolTable(TableType::TEMPVAR, "temp"));//��ʱ�������ű� [1] //�������� ���ޱ�ʶ������ �������display����
	displayTable.push_back(0);//һ��ʼ�����ı�϶���ȫ�ַ��ű�(0)
	nextQuad = 0;//��һ����Ԫʽ���
	mainFuncLine = -1;// main������������ʼ��Ϊ-1

	//Ŀ��������� ��ʼ��
	targetCodeGenerator.getSymbolTables(symbolTables);
}
SemanticAnalyzer::~SemanticAnalyzer() {
	quadsPrinter.close();
}
/*
	�õ���һ����Ԫʽ����� ��1��ʼ
*/
int SemanticAnalyzer::getNextQuad() {
	return ++nextQuad;
}
string SemanticAnalyzer::getArgName(const Index& idx, bool isReturnVal) {
	string argName = symbolTables[idx.tableIdx].getVarName(idx.symbolIdx);
	if (isReturnVal == true) {//�Ǻ�������ֵ
		int symbolIdx = symbolTables[0].getSymbolIdx(symbolTables[idx.tableIdx].getName());
		Symbol& symbol = symbolTables[0].getSymbol(symbolIdx);
		argName = symbol.varName + "_param_" + to_string(idx.symbolIdx);//������_param_�β����
	}
	else {//����
		if (SymbolType::VAR == symbolTables[idx.tableIdx].getSymbolType(idx.symbolIdx)) {
			argName = symbolTables[idx.tableIdx].getName() + "_var_" + argName;//������_var_������
		}
	}
	return argName;
}
/*
	��ӡ��Ԫʽ
*/
void SemanticAnalyzer::printQuads() {
	for (auto iter = quads.begin(); iter != quads.end(); iter++) {
		quadsPrinter << setiosflags(ios::right);
		quadsPrinter << setw(3) << iter->first << "  (" << (iter->second).op << ", " << (iter->second).arg1 << ", " << (iter->second).arg2 << ", " << (iter->second).result << ")" << endl;
	}
} 

/*
	����Ԫʽ���������Ԫʽ
*/
void SemanticAnalyzer::setQuad(const Quad& quad) {
	quads[quad.idx] = quad;
}

/*
	������Ԫʽ ��ʱÿ�λ���ֵֻ��һ�� ���ǻ�����Ԫʽ��һ��ֻ��һ��
	��Щ��������Ԫʽ����ͬһ��ֵ
*/
void SemanticAnalyzer::backpatch(const vector<int>& backpatchQuad, int backpatchValue) {
	for (auto iter = backpatchQuad.begin(); iter != backpatchQuad.end(); iter++) {
		quads[*iter].result = to_string(backpatchValue);
	}
}
/*
	����һ��xx���ű�(�������ű�)
*/
void SemanticAnalyzer::createSymbolTable(TableType type, const string& name) {
	symbolTables.push_back(SymbolTable(type, name));//����xx���ű�
	int index = symbolTables.size() - 1;//��ǰxx���ű������
	displayTable.push_back(index);//����ǰ����������display�� ��ʾ���ڷ����ñ�
}
/*
	�﷨���� ��Լʱִ������������Լ��� ���ز���ֵ
	ÿ������ʽӦ���������嶯��  Ϊ�˷������ͷ��� �����һ������ʽ���з���
*/
bool SemanticAnalyzer::semanticCheck(vector<GrammarSymbol>& grammarSymbolAttrStack, const Production& prod) {
	GrammarSymbol symbol;
	if (prod.left == "ConstValue") {//ConstValue->const_int|const_float|const_char
		//��ʱҪΪ����ʽ��ߵķ��ս������һ������
		GrammarSymbol const_int = grammarSymbolAttrStack.back();//ȡ�ķ���������ջջ��Ԫ��

		symbol.name = prod.left;
		symbol.value = const_int.value;
	}
	else if (prod.left == "Factor" && prod.right[0] == "ConstValue") {//Factor->ConstValue
		//�˴���Ҫ������ʱ���� �����������ʱ�������ű�
		//��ʱҪΪ����ʽ��ߵķ��ս������һ������
		GrammarSymbol constValue = grammarSymbolAttrStack.back();//ȡ�ķ���������ջջ��Ԫ��

		symbol.name = prod.left;
		//symbol.value = T_x=varName
		//������ʱ����
		int symbolIdx = symbolTables[1].setSymbol(constValue.value);//����ʱ�������м�����ʱ��������������
		string varName = symbolTables[1].getVarName(symbolIdx);//�õ���ʱ������
		symbol.value = varName;
		symbol.idx.tableIdx = 1;
		symbol.idx.symbolIdx = symbolIdx;

		//�˴���Ҫ����һ����Ԫʽ
		setQuad(Quad{ getNextQuad(),":=",constValue.value,"-",varName });

		//Ŀ����� ������ֵָ��(addi $ti,$0,imm)
		targetCodeGenerator.addiImmToReg(constValue.value, symbol.idx);
	}
	else if (prod.left == "Factor" && prod.right[0] == "(") {//Factor->( Expression ) 
		//Expression��value���ԡ�Index���Ը���Factor
		//��ʱҪΪ����ʽ��ߵķ��ս������һ������
		GrammarSymbol expression = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 1];//Expression

		symbol.name = prod.left;
		symbol.value = symbolTables[expression.idx.tableIdx].getVarName(expression.idx.symbolIdx);
		symbol.idx = expression.idx;
	}
	else if (prod.left == "Factor" && prod.right[0] == "id") {//Factor->id FTYPE
		//������ʶ��id����Ҫ���м�� �����Ƿ���
		//��Ҫ����FTYPE->CallFunction|$ ��ftype�����ĸ�����ʽ��Լ����
		GrammarSymbol ftype = grammarSymbolAttrStack.back();
		GrammarSymbol& id = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 1];//�˴����������� ��Ҫ�޸�ֵ

		symbol.name = prod.left;

		//�ӵ�ǰ����������ʼ��� ���Ƿ�����ñ���
		int idx = -1;
		for (auto iter = displayTable.rbegin(); iter != displayTable.rend(); ++iter) {//display����xx���ű������
			SymbolTable st = symbolTables[*iter];//�õ�xx���ű�
			idx = st.getSymbolIdx(id.value);//ͨ���ķ�����ֵ�ڷ��ű��в���������
			if (idx != -1) {//�ҵ���
				id.idx.tableIdx = *iter;
				id.idx.symbolIdx = idx;
				break;
			}
		}
		if (idx == -1) {
			cerr << "Semantic Analysis ERROR!" << endl;
			cerr << "Identifier " << id.value << " was not defined! Please check it!" << endl;
			return false;//����
		}

		//��ʶ���Ѿ�����,����Ǻ��������Ǳ�����
		if (ftype.value == "") {//������  FTYPE->$ => Factor->id
			//���id�Ƿ�����ʱ���� ͨ�����ڱ������ҵ��÷�������
			if (symbolTables[id.idx.tableIdx].getSymbolType(id.idx.symbolIdx) == SymbolType::TEMP_VAR) {//����ʱ����
				symbol.value = symbolTables[1].getVarName(id.idx.symbolIdx);//����ʱ���������ҵ�������
				symbol.idx = id.idx;
			}
			else {//������ʱ����
				//������ʱ����
				int symbolIdx = symbolTables[1].setSymbol(id.value);//����ʱ�������м�����ʱ��������������
				string varName = symbolTables[1].getVarName(symbolIdx);//�õ���ʱ������
				symbol.value = varName;
				symbol.idx = { 1,symbolIdx };

				//�˴���Ҫ����һ����Ԫʽ varName = getArgName()
				setQuad(Quad{ getNextQuad(),":=",getArgName(id.idx),"-",varName });

				//Ŀ����� ��ֵָ��(add $t1,$t2,$0) 
				//$t1Ϊ�������ʱ���� $2Ϊͨ���������ҷ��ű� ���� �ҵ��ļĴ���
				//(�Ĵ��������Ѿ���ֵ�� ��ûֵ���ŵ�regIdx����=-1 ��Ҫ���ڴ��п��ٿռ� ����ֵ������ʱ����)
				string rs = targetCodeGenerator.getOperand(symbol.idx);
				string rt = targetCodeGenerator.getOperand(id.idx);

				targetCodeGenerator.setInstruction({ "add",rs,rt,"$0" });
			}
		}
		else {//FTYPE->CallFunction  => Factor->id CallFunction //������
			//������ʱ���� �ø���ʱ�����洢��������ֵ
			int symbolIdx = symbolTables[1].setSymbol(id.value);
			string varName = symbolTables[1].getVarName(symbolIdx);
			//�õ���ǰ��������ֵ
			int tableIdx = symbolTables[id.idx.tableIdx].getSymbol(id.idx.symbolIdx).funcTableIdx;//�������ű������
			Index idx = { tableIdx, 0 };//����ֵ����ں������ű��0����

			//�˴���Ҫ����һ����Ԫʽ
			//����������ֵ������ʱ����
			setQuad(Quad{ getNextQuad(),":=",getArgName(idx),"-",varName });

			symbol.value = "";
			symbol.idx = { 1,symbolIdx };//�浽��ʱ������

			//Ŀ����� 
			//��������ǰ��Ҫ����Ĵ���
			targetCodeGenerator.clearRegs();
			//���ɵ��ú���ָ��
			targetCodeGenerator.setInstruction({"jal","","",id.value});
			//�Ӻ������÷��� ����fp sp ָ��
			//spָ��fp
			targetCodeGenerator.setInstruction({ "add","$sp","$fp","$0" });
			//fp ָ����fp �����ڴ������¼ʱ ����$fp��ֵ���ڵ�ַΪ$sp���ڴ���  �ڽ��µ�fpָ��sp
			//�� ($fp)����ĵ�ַ���ǻ��¼����ʱ$sp��ֵ ��ô�ϵ�fp�ͱ�����($fp)��
			targetCodeGenerator.setInstruction({ "lw","$fp","($fp)","" });
			//$spָ����Ҫ���� 
			int paraNum = symbolTables[0].getSymbol(id.idx.symbolIdx).formalParaNum;
			targetCodeGenerator.incStackPointer(-1 * paraNum);

			//�˴���Ҫע�� �Ǻ������� ��Ҫ����������ֵ����$v0(or $v1)
			string rs = targetCodeGenerator.getOperand(symbol.idx);

			targetCodeGenerator.setInstruction({ "add",rs,"$v0","$0" });
		}
	}
	else if (prod.left == "FactorLoop" && prod.right[0] == "FactorLoop") {//FactorLoop->FactorLoop Factor *|FactorLoop Factor /
		//�˴���ҪΪFactor������ʱ����������
		//��ʱҪΪ����ʽ��ߵķ��ս������һ������
		GrammarSymbol factor = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 1];
		GrammarSymbol factorLoop = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 2];

		symbol.name = prod.left;

		if (factorLoop.value == "") {//FactorLoop->$ ==> FactorLoop->Factor *
			//�˴�����Ҫ������ʱ���� ֻ�轫 ����ջ����* /����value����
			//Factor��ֵ�Ѿ�����ʱ������ ֻ�轫Factor�������ʱ������ ��ͷ�����������idx����
			symbol.value = grammarSymbolAttrStack.back().name;//* /
			symbol.idx = factor.idx;
		}
		else {//FactorLoop->FactorLoop Factor *|FactorLoop Factor /
			//FactorLoop = Factor * ���ֵ����ʱ����
			//��ʱ��Ҫ���� FactorLoop �����ֵ1 �� Factor�����ֵ2  �˳����
			string arg1 = getArgName(factorLoop.idx);
			string arg2 = getArgName(factor.idx);

			string rs = targetCodeGenerator.getOperand(factorLoop.idx);
			string rt = targetCodeGenerator.getOperand(factor.idx);

			int  nextQuad = getNextQuad();
			if (factorLoop.value == "*") {
				//���ɼ�����Ԫʽ
				setQuad(Quad{ nextQuad,"*",arg1,arg2,arg1 });//arg1 = arg1*arg2
				//Ŀ����� �˷�ָ��(mult rs,rt mflo rs)
				targetCodeGenerator.setInstruction({ "mult",rs, rt,"" });
				targetCodeGenerator.setInstruction({ "mflo",rs,"","" });//ȡ��32λ
			}
			else if (factorLoop.value == "/") {
				setQuad(Quad{ nextQuad,"/",arg1,arg2,arg1 });//arg1 = arg1/arg2
				//Ŀ����� ����ָ��(div rs,rt mflo rs) 
				targetCodeGenerator.setInstruction({ "div",rs,rt,"" });
				targetCodeGenerator.setInstruction({ "mflo",rs,"","" });//ȡ��32λ
			}
			symbol.value = factorLoop.value;// * /
			symbol.idx = factorLoop.idx;
		}
	}
	else if (prod.left == "Item") {//Item->FactorLoop Factor
		GrammarSymbol factorLoop = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 1];
		GrammarSymbol factor = grammarSymbolAttrStack.back();

		symbol.name = prod.left;
		//�ж�FactorLoop�Ƿ�Ϊ��
		if (factorLoop.value == "") {//FactorLoop->$ =>Item -> Factor
			//�˴�����������ʱ���� ֱ�ӽ�Factor����ʱ����ֵ����Item����(��ʱ������ֵֻ�贫��ͷ�����������)
			symbol.value = "";
			symbol.idx = factor.idx;
		}
		else {//FactorLoop����
			//FactorLoop�е� Factor ��ʱ����ֵ 
			//Factor �е� ��ʱ����ֵ
			//����FactorLoop.value = */ �������� ���Ҫ����arg1��
			string arg1 = getArgName(factorLoop.idx);
			string arg2 = getArgName(factor.idx);

			string rs = targetCodeGenerator.getOperand(factorLoop.idx);
			string rt = targetCodeGenerator.getOperand(factor.idx);

			int  nextQuad = getNextQuad();
			if (factorLoop.value == "*") {
				//���ɼ�����Ԫʽ
				setQuad(Quad{ nextQuad,"*",arg1,arg2,arg1 });//arg1 = arg1*arg2
				//Ŀ����� �˷�ָ��(mult rs,rt mflo rs)
				targetCodeGenerator.setInstruction({ "mult",rs, rt,"" });
				targetCodeGenerator.setInstruction({ "mflo",rs,"","" });//ȡ��32λ
			}
			else if (factorLoop.value == "/") {
				setQuad(Quad{ nextQuad,"/",arg1,arg2,arg1 });//arg1 = arg1/arg2
				//Ŀ����� ����ָ��(div rs,rt mflo rs) 
				targetCodeGenerator.setInstruction({ "div",rs,rt,"" });
				targetCodeGenerator.setInstruction({ "mflo",rs,"","" });//ȡ��32λ

			}
			//������ֵ����Item��
			symbol.value = factorLoop.value;// * /
			symbol.idx = factorLoop.idx;
		}
	}
	else if (prod.left == "ItemLoop" && prod.right[0] == "ItemLoop") {//ItemLoop->ItemLoop Item +|ItemLoop Item -
		GrammarSymbol itemLoop = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 2];
		GrammarSymbol item = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 1];

		symbol.name = prod.left;
		//�ж�ItemLoop�Ƿ�Ϊ��
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
				//���ɼ�����Ԫʽ
				setQuad(Quad{ nextQuad,"+",arg1,arg2,arg1 });//arg1 = arg1+arg2
				//Ŀ����� �ӷ�ָ��(add rs,rt,rd)(rs = rt+rd)
				targetCodeGenerator.setInstruction({ "add",rs,rs,rt });
			}
			else if (itemLoop.value == "-") {
				//���ɼ�����Ԫʽ
				setQuad(Quad{ nextQuad,"-",arg1,arg2,arg1 });//arg1 = arg1-arg2
				//Ŀ����� ����ָ��(sub rs,rt,rd)(rs = rt-rd)
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
		//�ж�ItemLoop�Ƿ�Ϊ��
		if (itemLoop.value == "") {//AddExpression->Item
			//ֱ�Ӵ�����ʱ��������
			//�˴�����������ʱ���� ֱ�ӽ�Item����ʱ����ֵ����AddExpression����(��ʱ������ֵֻ�贫��ͷ�������)
			symbol.value = "";
			symbol.idx = item.idx;
		}
		else {//ItemLoop����
		//ItemLoop�е� Item ��ʱ����ֵ 
		//Item �е� ��ʱ����ֵ
		//����ItemLoop.value = +- �������� ���Ҫ����arg1��
			string arg1 = getArgName(itemLoop.idx);
			string arg2 = getArgName(item.idx);

			string rs = targetCodeGenerator.getOperand(itemLoop.idx);
			string rt = targetCodeGenerator.getOperand(item.idx);

			int  nextQuad = getNextQuad();
			if (itemLoop.value == "+") {
				//���ɼ�����Ԫʽ
				setQuad(Quad{ nextQuad,"+",arg1,arg2,arg1 });//arg1 = arg1+arg2
				//Ŀ����� �ӷ�ָ��(add rs,rt,rd)(rs = rt+rd)
				targetCodeGenerator.setInstruction({ "add",rs,rs,rt });
			}
			else if (itemLoop.value == "-") {
				setQuad(Quad{ nextQuad ,"-",arg1,arg2,arg1 });//arg1 = arg1-arg2
				//Ŀ����� ����ָ��(sub rs,rt,rd)(rs = rt-rd)
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
			//���贴����ʱ���� ֱ�ӽ���AddExpression��ֵ���д���
			symbol.value = grammarSymbolAttrStack.back().value;//> < >= <= ....
			symbol.idx = addExpression.idx;
		}
		else {//AddExpressionLoop->AddExpressionLoop AddExpression Relop
			//�˴���Ҫ������Ԫʽ
			/*
			��Ԫʽ�����ɹ������£�
				nextquad (jop,X1,X2, 0); trueList
				nextquad+1 (j  ,- ,- , 0); falseList
			�˴��Ǵ��������Ԫʽ
			*/
			string arg1 = getArgName(addExpressionLoop.idx);
			string arg2 = getArgName(addExpression.idx);

			string rs = targetCodeGenerator.getOperand(addExpressionLoop.idx);
			string rt = targetCodeGenerator.getOperand(addExpression.idx);

			//��Ԫʽ
			int nextQuadTrue = getNextQuad();
			symbol.trueList.push_back(nextQuadTrue);
			setQuad(Quad{ nextQuadTrue,"j" + addExpressionLoop.value,arg1,arg2,"0" });//�����

			//Ŀ����� �Ƚ���תָ�� ����cmp rs,rt,branchLabel��
			int inst;
			if (addExpressionLoop.value == ">") {//branchLabel �ȿ���  ֮����Ҫ����
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
			//�����ָ��  ������
			symbol.trueLabel.push_back(inst);


			int nextQuadFalse = getNextQuad();
			symbol.falseList.push_back(nextQuadFalse);
			setQuad(Quad{ nextQuadFalse,"j","-","-","0" });//�ٳ���

			//Ŀ����� �ٳ��ڵ���תָ�� ������
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
	else if (prod.left == "Expression") {//Expression->AddExpressionLoop AddExpression ��ʵͬǰ���AddExpressLoop����ʽ
		GrammarSymbol addExpression = grammarSymbolAttrStack.back();
		GrammarSymbol addExpressionLoop = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 1];

		symbol.name = prod.left;
		//�ж�AddExpressionLoop�Ƿ�Ϊ��
		if (addExpressionLoop.value == "") {//AddExpressionLoop -> $ ==> Expression->AddExpression
			//���贴����ʱ���� ֱ�Ӵ������Լ���
			symbol.value = addExpression.value;
			symbol.idx = addExpression.idx;
		}
		else {//����
			//�˴���Ҫ������Ԫʽ
			/*
			��Ԫʽ�����ɹ������£�
				nextquad (jop,X1,X2, 0); trueList
				nextquad+1 (j  ,- ,- , 0); falseList
			�˴��Ǵ��������Ԫʽ
			*/
			string arg1 = getArgName(addExpressionLoop.idx);
			string arg2 = getArgName(addExpression.idx);

			string rs = targetCodeGenerator.getOperand(addExpressionLoop.idx);
			string rt = targetCodeGenerator.getOperand(addExpression.idx);

			int nextQuadTrue = getNextQuad();
			symbol.trueList.push_back(nextQuadTrue);
			setQuad(Quad{ nextQuadTrue,"j" + addExpressionLoop.value,arg1,arg2,"0" });//�����

			//Ŀ����� �Ƚ���תָ�� ����cmp rs,rt,branchLabel��
			int inst;
			if (addExpressionLoop.value == ">") {//branchLabel �ȿ���  ֮����Ҫ����
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
			//�����
			symbol.trueLabel.push_back(inst);

			int nextQuadFalse = getNextQuad();
			symbol.falseList.push_back(nextQuadFalse);
			setQuad(Quad{ nextQuadFalse,"j","-","-","0" });//�ٳ���

			//Ŀ����� �ٳ�����תָ��
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
		this->nextQuad--;//��ʱnextQuadҪ-1 ������һ����Ԫʽ��Ų���

		//����ifǰ ��ռĴ���
		targetCodeGenerator.clearRegs();

		this->label++;//����if��� ���+1
		labelStack.push_back(label);
		int inst = targetCodeGenerator.setInstruction({ "__if"+to_string(labelStack.back()),":","","" }); //��������ָ������
		//__if0
		symbol.inst = inst;
	}
	else if (prod.left == "If_M2") {// If_M2->$ 
		symbol.name = prod.left;
		symbol.value = "";
		symbol.quad = getNextQuad();//M.quad = nextquad;
		this->nextQuad--;//��ʱnextQuadҪ-1 ������һ����Ԫʽ��Ų���
		
		//����elseǰ��ռĴ���
		targetCodeGenerator.clearRegs();

		//else ��֧ label����Ҫ+1 ȥ��ǰif�����ǩ����
		int inst = targetCodeGenerator.setInstruction({ "__else" + to_string(labelStack.back()),":","","" }); //��������ָ������
		//__else0
		symbol.inst = inst;//�Ժ����
	}
	else if (prod.left == "If_N") {// If_N->$
		symbol.name = prod.left;
		symbol.value = "";
		int nextQuad = getNextQuad();
		symbol.nextList.push_back(nextQuad);
		//����һ����Ԫʽ
		setQuad(Quad{ nextQuad,"j","-","-","-" });//������

		//�˳�if else ��� ��ռĴ���
		targetCodeGenerator.clearRegs();

		//Ŀ����� ��ִֹ��else
		int inst = targetCodeGenerator.setInstruction({ "j","","","" });
		
		symbol.nextLabel.push_back(inst);
	}
	else if (prod.left == "ElseSentenceBlock" && prod.right[0] == "If_N") {//ElseSentenceBlock->$|If_N else If_M2 SentenceBlock
		GrammarSymbol sentenceBlock = grammarSymbolAttrStack.back();
		GrammarSymbol if_n = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 3];
		GrammarSymbol if_m2 = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 1];

		symbol.name = prod.left;
		symbol.inst = if_m2.inst;
		symbol.value = to_string(sentenceBlock.quad);//sentenceBlock.quad�ز��� ��Լʱ����Ifsentence.nextlist
		symbol.quad = if_m2.quad;//��If_M2��ֵ����elseSentenceBlock��quad����  ��if...else...�����Ҫ����If_M2.quad
		//merge(N.nextlist,S2.nextlist)
		symbol.nextList.insert(symbol.nextList.end(), sentenceBlock.nextList.begin(), sentenceBlock.nextList.end());
		symbol.nextList.insert(symbol.nextList.end(), if_n.nextList.begin(), if_n.nextList.end());
		//����ָ��
		symbol.nextLabel.insert(symbol.nextLabel.end(), sentenceBlock.nextLabel.begin(), sentenceBlock.nextLabel.end());
		symbol.nextLabel.insert(symbol.nextLabel.end(), if_n.nextLabel.begin(), if_n.nextLabel.end());

		//�˳�elseǰ ��ռĴ���
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
			//if ... then ...��Ҫ��sentenceBlock.quad ����symbol.nextlist 
			backpatch(symbol.nextList, sentenceBlock.quad);
			////////////////////////////////////////////////////////////////////////////////////////////////////////
			
			//�˳�ֻ��if�����ǰ ��Ҫ��ռĴ���
			targetCodeGenerator.clearRegs();

			//Ŀ��������
			symbol.nextLabel.insert(symbol.nextLabel.end(), expression.falseLabel.begin(), expression.falseLabel.end());
			symbol.nextLabel.insert(symbol.nextLabel.end(), sentenceBlock.falseLabel.begin(), sentenceBlock.falseLabel.end());
			//����ָ��
			targetCodeGenerator.backpatch_(expression.trueLabel,if_m1.inst);
			int inst = targetCodeGenerator.setInstruction({ "__end_if"+to_string(labelStack.back()),":","","" });//�˴�ֻ��if��� ������ǩ
			targetCodeGenerator.backpatch_(symbol.nextLabel, inst);
			//if������֮��labelStack��Ҫ��ջ
			labelStack.pop_back();
		}
		else {//IfSentence->if ( Expression ) If_M1 SentenceBlock1 If_N else If_M2 SentenceBlock2
			//merge(S1.nextlist,N.nextlist,S2.nextlist)
			symbol.nextList.insert(symbol.nextList.end(), sentenceBlock.nextList.begin(), sentenceBlock.nextList.end());
			symbol.nextList.insert(symbol.nextList.end(), elseSentenceBlock.nextList.begin(), elseSentenceBlock.nextList.end());
			//����
			//backpatch(E.truelist,M1.quad)
			//backpatch(E.falselist,M2.quad)
			backpatch(expression.trueList, if_m1.quad);
			backpatch(expression.falseList, elseSentenceBlock.quad);
			//if ... then ...��Ҫ��elseSentenceBlock.value ����symbol.nextlist
			backpatch(symbol.nextList, stoi(elseSentenceBlock.value));
			////////////////////////////////////////////////////////////////////////////////////////////////////////
			//Ŀ��������
			symbol.nextLabel.insert(symbol.nextLabel.end(), sentenceBlock.nextLabel.begin(), sentenceBlock.nextLabel.end());
			symbol.nextLabel.insert(symbol.nextLabel.end(), elseSentenceBlock.nextLabel.begin(), elseSentenceBlock.nextLabel.end());
			
			targetCodeGenerator.backpatch_(expression.trueLabel, if_m1.inst);
			targetCodeGenerator.backpatch_(expression.falseLabel, elseSentenceBlock.inst);//if_m2.inst����
			//����nextlabel
			int inst = targetCodeGenerator.setInstruction({"__end_if_else"+to_string(labelStack.back()),":","",""});
			targetCodeGenerator.backpatch_(symbol.nextLabel,inst);

			//if else ������ ��ջ
			labelStack.pop_back();
;		}
	}
	else if (prod.left == "While_M1") {// While_M1 -> $ 
		symbol.name = prod.left;
		symbol.value = "";
		symbol.quad = getNextQuad();//M.quad = nextquad;
		this->nextQuad--;//��ʱnextQuadҪ-1 ������һ����Ԫʽ��Ų���

		//Ŀ��������
		//����ѭ��ǰ ��ռĴ���
		targetCodeGenerator.clearRegs();

		//����ѭ����� ��ǩ+1
		this->label++;
		labelStack.push_back(label);
		int inst = targetCodeGenerator.setInstruction({"__while"+to_string(labelStack.back()),":","",""});
		symbol.inst = inst;
	}
	else if (prod.left == "While_M2") {// While_M2 -> $ 
		symbol.name = prod.left;
		symbol.value = "";
		symbol.quad = getNextQuad();//M.quad = nextquad;
		this->nextQuad--;//��ʱnextQuadҪ-1 ������һ����Ԫʽ��Ų���
		
		//Ŀ��������
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
		//����
		//backpatch(E.truelist,M2.quad)
		//backpatch(S.nextlist,M1.quad)
		backpatch(expression.trueList, while_m2.quad);
		backpatch(sentenceBlock.nextList, while_m1.quad);
		//S.nextlist = E.falselist
		symbol.nextList = expression.falseList;

		//Ŀ��������
		targetCodeGenerator.backpatch_(expression.trueLabel, while_m2.inst);
		targetCodeGenerator.backpatch_(sentenceBlock.nextLabel, while_m1.inst);//��ʵsentenceBlock��nextList  nextLabel��Ϊ��
		symbol.nextLabel = expression.falseLabel;


		//����һ����Ԫʽ
		//ע�����Ԫʽ����SentenceBlock��  ���ǿ���ѭ����
		setQuad(Quad{ getNextQuad(),"j","-","-",to_string(while_m1.quad) });

		//��������ѭ��ǰ ��ռĴ���
		targetCodeGenerator.clearRegs();

		//Ŀ����� ����ѭ��
		targetCodeGenerator.setInstruction({ "j","","",targetCodeGenerator.getInst(while_m1.inst).op });//j __while


		//��SentenceBlock.quad + 1���� WhileSentence.nextlist ��ʾ�˳�ѭ��
		//��Ҫ�Ƕ���һ��(j,-,-,M1.quad) ����������SentenceBlock�����ɵ���Ԫʽ Ҫ������һ��
		backpatch(symbol.nextList, sentenceBlock.quad + 1);

		int inst = targetCodeGenerator.setInstruction({"__while_end"+to_string(labelStack.back()),":","",""});
		//Ŀ�������� while��nextlabel
		targetCodeGenerator.backpatch_(symbol.nextLabel, inst);

		//�˳�ѭ�� labelStack ��ջ
		labelStack.pop_back();
		
	}
	else if (prod.left == "AssignSentence") {//	AssignSentence->id = Expression ;
		GrammarSymbol expression = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 1];
		GrammarSymbol id = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 3];

		symbol.name = prod.left;
		//����identifier��Ҫ����Ƿ����  (��ǰ�������ű��ȫ�ַ��ű�)
		int symbolIdx = -1;
		int tableIdx = 0;
		for (auto iter = displayTable.rbegin(); iter != displayTable.rend(); ++iter) {
			SymbolTable st = symbolTables[*iter];//�õ�xx���ű�
			symbolIdx = st.getSymbolIdx(id.value);//ͨ���ķ�����ֵ�ڷ��ű��в���������
			if (symbolIdx != -1) {//�ҵ���
				tableIdx = *iter;
				break;
			}
		}
		if (symbolIdx == -1) {//û����� ����
			cerr << "Semantic Analysis ERROR!" << endl;
			cerr << "Identifier " << id.value << " was not defined! Please check it!" << endl;
			return false;//����
		}
		//��ʶ���Ѿ�����
		string arg1 = getArgName(expression.idx);
		string result = getArgName(Index{ tableIdx,symbolIdx });
		//����һ����Ԫʽ
		setQuad(Quad{ getNextQuad(),":=",arg1,"-",result });

		//��ֵָ�� add rs,rt,$0 //rs = rt;
		string rs = targetCodeGenerator.getOperand(Index{ tableIdx,symbolIdx });
		string rt = targetCodeGenerator.getOperand(expression.idx);
		targetCodeGenerator.setInstruction({ "add",rs,rt,"$0" });


		symbol.value = "";//��
	}
	else if (prod.left == "ReturnExpression" && prod.right[0] == "$") {//ReturnExpression->$
		symbol.name = prod.left;
		symbol.value = "";//����ֵ����Ϊ��

		//ReturnSentence->return ReturnExpression($) ;
		//��ʱջ����return
		//����������void 
		//�жϵ�ǰ�������ű���0������ ����ֵ����
		if (VarType::VOID != symbolTables[displayTable.back()].getVarType(0)) {
			cerr << "Semantic Analysis ERROR!" << endl;
			string funcName = symbolTables[displayTable.back()].getName();
			cerr << "The return value type of the function \'"<<funcName<<"\' does not match the declared type! Please check it!" << endl;
			return false;
		}
	}
	else if (prod.left == "ReturnExpression" && prod.right[0] == "Expression") {//ReturnExpression->Expression
		//ReturnSentence->return ReturnExpression ;
		//��ʱ����ֵ����void
		if (VarType::INT != symbolTables[displayTable.back()].getVarType(0)) {
			cerr << "Semantic Analysis ERROR!" << endl;
			string funcName = symbolTables[displayTable.back()].getName();
			cerr << "The return value type of the function \'"<<funcName<<"\' does not match the declared type! Please check it!" << endl;
			return false;
		}
		GrammarSymbol expression = grammarSymbolAttrStack.back();

		symbol.name = prod.left;
		symbol.value = "int";//����ֵ��int
		//�˴�Expression�������ʱ��������ֱ�Ӵ���ReturnExpression
		symbol.idx = expression.idx;

		//���� ����ֵ ����$v0 ��
		string rt = targetCodeGenerator.getOperand(expression.idx);
		targetCodeGenerator.setInstruction({"add","$v0",rt,"$0"});
		// �������ص�ַ���� $ra
		targetCodeGenerator.setInstruction({"lw","$ra","4($fp)",""});//fp + 4 ��ŷ��ص�ַ
	}
	else if (prod.left == "ReturnSentence") {//ReturnSentence->return ReturnExpression ;
		GrammarSymbol returnExpression = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 1];

		symbol.name = prod.left;

		if (returnExpression.value == "") {//����void����ֵ
			symbol.value = "";
		}
		else {//����int ����ֵ
			symbol.value = "int";
			//������ֵ���ں������ű��0����
			symbol.idx = { Index{displayTable.back(),0} };
			string arg1 = getArgName(returnExpression.idx);//ReturnExpression�洢����ʱ����
			string result = getArgName(symbol.idx);

			//����һ������ֵ��ֵ��Ԫʽ
			setQuad(Quad{ getNextQuad(),":=",arg1,"-",result });
		}
		//����һ������ֵ��Ԫʽ �˴��Ǳ�ʾ�˳��ú�������
		//(return,-,-,function)
		setQuad(Quad{ getNextQuad(),"return","-","-",symbolTables[displayTable.back()].getName() });
	}
	else if (prod.left == "InternalVariableStmt") {//InternalVariableStmt->int id
		symbol.name = prod.left;

		GrammarSymbol id = grammarSymbolAttrStack.back();
		//����identifier��Ҫ���Ƕ��ں����ж����
		//�ڵ�ǰ�������ű���(displayTable.back())���Ҹñ�ʶ��  ���Ƿ����
		if (symbolTables[displayTable.back()].getSymbolIdx(id.value) != -1) {//�ҵ� ˵���ñ�ʶ���Ѿ����� �ظ����� 
			cerr << "Semantic Analysis ERROR!" << endl;
			cerr << "Identifier " << id.value << " has been defined! Please check it!" << endl;
			return false;//����
		}
		//��ȫ�ַ��ű��в��Ҹñ�ʶ�� ���Ƿ���� �Ƿ��뺯������ͻ
		int symbolIdx1 = symbolTables[0].getSymbolIdx(id.value);
		if (-1 != symbolIdx1 && symbolTables[0].getSymbolType(symbolIdx1) == SymbolType::FUNCTION) {//�Ƿ񱻶���Ϊ����
			cerr << "Semantic Analysis ERROR!" << endl;
			cerr << "Identifier " << id.value << " has been defined as a FUNCTION! Please check it!" << endl;
			return false;//����
		}
		//û����� Ҳ���뺯������ͻ ���Զ���Ϊ����
		Symbol var;
		var.type = SymbolType::VAR;
		var.varName = id.value;
		var.varValue = "";//�� ��δ��ֵ
		var.varType = VarType::INT;
		//���ñ������뵱ǰ�������ű�
		int  symbolIdx2 = symbolTables[displayTable.back()].setSymbol(var); //���ر����ں������ű�������

		symbol.value = id.value;//������x
		symbol.idx = { displayTable.back(), symbolIdx2 };

		targetCodeGenerator.incStackPointer(1);//sp = sp + 4; Ϊ�ֲ���������ռ�
	}
	else if (prod.left == "SentenceBlock") {//SentenceBlock->SB_M { InternalStmt SentenceString }
		//���ڻ��� �����﷨��λ��nextlist
		symbol.name = prod.left;
		symbol.value = "sb";//����
		symbol.quad = getNextQuad();//����
		this->nextQuad--;//�˴�����-1

		////�˴�����Ҫ����һ����ǩ ��ʾ����
		//int inst = targetCodeGenerator.setInstruction({ "__end",":","","" });
		//symbol.inst = inst;//����
	}
	else if (prod.left == "Param") {//Param->int id  �β�
		GrammarSymbol id = grammarSymbolAttrStack.back();

		symbol.name = prod.left;
		//����identifier��Ҫ���Ƿ����
		//�ڵ�ǰ�������ű���(displayTable.back())���Ҹñ�ʶ��  ���Ƿ����
		if (symbolTables[displayTable.back()].getSymbolIdx(id.value) != -1) {//�ҵ� ˵���ñ�ʶ���Ѿ����� �ظ����� 
			cerr << "Semantic Analysis ERROR!" << endl;
			cerr << "Identifier " << id.value << " has been defined! Please check it!" << endl;
			return false;//����
		}
		//��ȫ�ַ��ű��в��Ҹñ�ʶ�� ���Ƿ���� �Ƿ��뺯������ͻ
		int symbolIdx1 = symbolTables[0].getSymbolIdx(id.value);
		if (-1 != symbolIdx1 && symbolTables[0].getSymbolType(symbolIdx1) == SymbolType::FUNCTION) {//�Ƿ񱻶���Ϊ����
			cerr << "Semantic Analysis ERROR!" << endl;
			cerr << "Identifier " << id.value << " has been defined as a FUNCTION! Please check it!" << endl;
			return false;//����
		}
		//û����� Ҳ���뺯������ͻ ���Զ���Ϊ����
		Symbol var;
		var.type = SymbolType::VAR;
		var.varName = id.value;
		var.varValue = "";//�� ��δ��ֵ
		var.varType = VarType::INT;
		//���ñ������뵱ǰ�������ű�
		int  symbolIdx2 = symbolTables[displayTable.back()].setSymbol(var); //���ر����ں������ű�������

		symbol.value = id.value;//������x
		symbol.idx = { displayTable.back(), symbolIdx2 };

		//ȫ�ַ��ű��еĵ�ǰ�������� ��Ҫ����ʽ����������1
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
		//��ʱ��ջ��Ԫ��Ϊid, FunctionStmt��û��ջ
		GrammarSymbol id = grammarSymbolAttrStack.back();
		GrammarSymbol returnType = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 1];
		//��ȫ�ַ��ű��в��Ҵ˺����Ƿ���
		if (-1 != symbolTables[0].getSymbolIdx(id.value)) {
			cerr << "Semantic Analysis ERROR!" << endl;
			cerr << "Identifier " << id.value << " has been defined as a FUNCTION! Please check it!" << endl;
			return false;//����
		}
		//û�ж��� ����һ���º������ű�
		createSymbolTable(TableType::FUNCTION, id.value);
		//���ú������ӵ�ȫ�ַ��ű���
		Symbol func;
		func.type = SymbolType::FUNCTION;
		func.varName = id.value;
		func.varValue = "";
		func.varType = (returnType.value == "void") ? VarType::VOID : VarType::INT;
		func.funcTableIdx = symbolTables.size() - 1;//����������ȫ�ַ��ű��е�λ��
		int symbolIdx = symbolTables[0].setSymbol(func);

		//������������ֵ
		Symbol returnVar;
		returnVar.type = SymbolType::RETURN;
		returnVar.varName = symbolTables[displayTable.back()].getName() + "_return_value";
		returnVar.varValue = "";//����ֵ��ʱΪ��
		returnVar.varType = (returnType.value == "void") ? VarType::VOID : VarType::INT;
		//����������ֵ���뵽�������ű��0�������ñ����Ǵ�������ʱ��һ���ӽ�ȥ�Ĺ�����Ϊ0��
		symbolTables[displayTable.back()].setSymbol(returnVar);

		int nextQuad = getNextQuad();
		//�жϸñ�ʶ���Ƿ�Ϊmain
		if (id.value == "main") {
			mainFuncLine = nextQuad;
		}

		//����һ����Ԫʽ ���ڱ�ʾ��ʱ���뵱ǰ����
		setQuad(Quad{ nextQuad, id.value,"-","-","-" });

		//Ŀ����� ��ʾ����ú���
		targetCodeGenerator.setInstruction({ id.value,":","","" });
		//�������¼
		targetCodeGenerator.setActivityRecordHeader();
	}
	else if (prod.left == "FunctionEnd") {//FunctionEnd->$
		symbol.name = prod.left;
		symbol.value = "";

		//�õ���ǰ�������� �����ж��Ƿ���main����
		string funcName = symbolTables[displayTable.back()].getName();

		//�˴�Ҫ�ر�ע�� Ҫ��display�����˳����������ű�
		displayTable.pop_back();

		targetCodeGenerator.clearRegs();//�˳�����ǰ ��ռĴ���

		if (funcName == "main") {
			targetCodeGenerator.setInstruction({ "j","","","Exit" }); //go to Exit
			targetCodeGenerator.setInstruction({ "Exit",":","","" });
		}
		else {//��ͨ����  jr $ra
			targetCodeGenerator.setInstruction({ "jr","$ra","","" }); //go to $31�Ĵ����洢�ķ��ص�ַ
		}
	}
	else if (prod.left == "StmtType" && prod.right[0] == "VariableStmt") {//StmtType->VariableStmt
		//��ʱֱ�ӽ�VariableStmt�����Դ��ݼ���
		symbol.name = prod.left;
		symbol.value = grammarSymbolAttrStack.back().value;//;
	}
	else if (prod.left == "Stmt") {//Stmt->int id StmtType
		GrammarSymbol id = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 1];
		GrammarSymbol stmtType = grammarSymbolAttrStack.back();

		symbol.name = prod.left;

		//�ж�StmtType
		if (stmtType.value == ";") {//�������
			//�ӵ�ǰ����������ʼ��� ���Ƿ�����ñ���
			int idx = -1;
			for (auto iter = displayTable.rbegin(); iter != displayTable.rend(); ++iter) {//display����xx���ű������
				SymbolTable st = symbolTables[*iter];//�õ�xx���ű�
				idx = st.getSymbolIdx(id.value);//ͨ���ķ�����ֵ�ڷ��ű��в���������
				if (idx != -1) {//�ҵ��� ˵���Ѿ��������
					cerr << "Semantic Analysis ERROR!" << endl;
					cerr << "Identifier " << id.value << " has been defined! Please check it!" << endl;
					return false;//����
				}
			}
			//û�ж����
			Symbol var;
			var.type = SymbolType::VAR;
			var.varName = id.value;
			var.varValue = "";//��,��δ��ֵ
			var.varType = VarType::INT;
			symbolTables[displayTable.back()].setSymbol(var);

			symbol.value = id.value;

			targetCodeGenerator.incStackPointer(1);//sp = sp + 1; ����һ���ֲ����� Ϊ�����洢�ռ�
		}
		//else {//���庯�� FunctionBegin->$ʱ�Ѿ����
		//}
	}
	else if (prod.left == "CallCheck") {//CallCheck->$
		//���ú����Ƿ���
		//Factor->id ( CallCheck($) ActualParamList )   ActualParamListδ��ջ
		GrammarSymbol id = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 1];

		symbol.name = prod.left;
		//����id�Ƿ���
		int idx = -1;
		for (auto iter = displayTable.rbegin(); iter != displayTable.rend(); ++iter) {//display����xx���ű������
			SymbolTable st = symbolTables[*iter];//�õ�xx���ű�
			idx = st.getSymbolIdx(id.value);//ͨ���ķ�����ֵ�ڷ��ű��в���������
			if (idx != -1) {//�ҵ���
				break;
			}
		}
		if (idx == -1) {
			cerr << "Semantic Analysis ERROR!" << endl;
			cerr << "Identifier " << id.value << " was not defined! Please check it!" << endl;
			return false;//����
		}
		//������ ���Ƿ�Ϊ��������
		if (SymbolType::FUNCTION != symbolTables[0].getSymbolType(idx)) {
			//���Ǻ������� ����
			cerr << "Semantic Analysis ERROR!" << endl;
			cerr << "Identifier " << id.value << " is not a FUNCTION, it is not callable! Please check it!" << endl;
			return false;
		}
		symbol.value = "";
		//�ѱ����ú������ŵ�����
		symbol.idx = { 0,idx };//{displayTable[*iter],idx}
	}
	else if (prod.left == "ExpressionLoop" && prod.right[0] == "$") {//ExpressionLoop->$
		//ActualParamList->ExpressionLoop($) Expression
		symbol.name = prod.left;
		symbol.value = "1";//����1ʵ�Ρ���Expression
	}
	else if (prod.left == "ExpressionLoop" && prod.right[0] == "ExpressionLoop") { //ExpressionLoop -> ExpressionLoop Expression ,
		//ActualParamList->ExpressionLoop Expression , 
		//��ʱջ��Ϊ ,
		GrammarSymbol expression = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 1];
		GrammarSymbol expressionLoop = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 2];
		GrammarSymbol callCheck = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 3];

		symbol.name = prod.left;

		//��CallCheck���ҵ���ŵı����ú�������
		Symbol& calledFunc = symbolTables[callCheck.idx.tableIdx].getSymbol(callCheck.idx.symbolIdx);
		int passedParamNum = stoi(expressionLoop.value);

		//�˴�������в��������ж� ��Ϊ��ʱ���ڽ���ʵ��ɨ��
		//Ӧ���ڹ�ԼΪActualListʱ�����ж� ��ʱ�����Ѿ�ɨ����� 

		/*int formalParamNum = calledFunc.formalParaNum;
		int passedParamNum = stoi(expressionLoop.value);
		if (formalParamNum < passedParamNum) {
			cerr << "Semantic Analysis ERROR!" << endl;
			cerr << "More parameters given!" << endl;
			cerr << "FUNCTION " << calledFunc.varName << " expected " << formalParamNum <<
				" parameter(s), but " << passedParamNum << " given! Please check it!" << endl;
			return false;
		}*/

		Index idx = { calledFunc.funcTableIdx,passedParamNum };//0�Ƿ���ֵ
		string result = getArgName(idx, true);//����ֵ
		string arg1 = getArgName(expression.idx);
		//����һ��ʵ�θ�ֵ��Ԫʽ
		//func(a,b)
		//(:=,T1,-,func_param_a)
		//(:=,T2,-,func_param_b)
		//(call,-,-,func)
		setQuad(Quad{ getNextQuad(),":=",arg1,"-",result });

		//Ŀ�����
		string rs = targetCodeGenerator.getOperand(expression.idx);
		targetCodeGenerator.setInstruction({ "sw",rs,"($sp)","" });//a = T1  (T1 = imm)  Ϊ��ʱ��������洢�ռ�
		targetCodeGenerator.incStackPointer(1);//sp  = sp + 4 

		//��һ������
		passedParamNum++;
		//ʵ�θ���+1
		symbol.value = to_string(passedParamNum);
	}
	else if (prod.left == "ActualParamList" && prod.right[0] == "$") {//ActualParamList->$
		//CallFunction-> ( CallCheck ActualParamList($)
		//��ʱջ��ΪCallCheck 
		GrammarSymbol callCheck = grammarSymbolAttrStack.back();

		symbol.name = prod.left;

		//��CallCheck���ҵ���ŵı����ú�������
		Symbol& calledFunc = symbolTables[callCheck.idx.tableIdx].getSymbol(callCheck.idx.symbolIdx);

		int formalParamNum = calledFunc.formalParaNum;//�ڱ����ú��������еõ��βθ���
		if (0 != formalParamNum) {
			cerr << "Semantic Analysis ERROR!" << endl;
			cerr << "None parameters given!" << endl;
			cerr << "FUNCTION " << calledFunc.varName << " expected " << formalParamNum <<
				" parameter(s), but none given! Please check it!" << endl;
			return false;
		}

		symbol.value = "0";//0ʵ��
	}
	else if (prod.left == "ActualParamList" && prod.right[0] == "ExpressionLoop") {
		//CallFunction->( CallCheck ExpressionLoop Expression
		GrammarSymbol expression = grammarSymbolAttrStack.back();
		GrammarSymbol expressionLoop = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 1];
		GrammarSymbol callCheck = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 2];

		symbol.name = prod.left;

		//��CallCheck���ҵ���ŵı����ú�������
		Symbol& calledFunc = symbolTables[callCheck.idx.tableIdx].getSymbol(callCheck.idx.symbolIdx);

		int formalParamNum = calledFunc.formalParaNum;//�βθ���
		int passedParamNum = stoi(expressionLoop.value);//ʵ�ʴ��ݵ�ʵ�θ���

		if (formalParamNum < passedParamNum) {//���ݲ�������
			cerr << "Semantic Analysis ERROR!" << endl;
			cerr << "More parameters given!" << endl;
			cerr << "FUNCTION " << calledFunc.varName << " expected " << formalParamNum <<
				" parameter(s), but " << passedParamNum << " given! Please check it!" << endl;
			return false;
		}

		//�˴���passedParamNumΪ���һ����������
		Index idx = { calledFunc.funcTableIdx,passedParamNum };
		string result = getArgName(idx, true);//����ֵ
		string arg1 = getArgName(expression.idx);
		//����һ��ʵ�θ�ֵ��Ԫʽ
		//func(a,b)
		//(:=,T1,-,func_param_a)
		//(:=,T2,-,func_param_b)
		//(call,-,-,func)

		setQuad(Quad{ getNextQuad(),":=",arg1,"-",result });

		//�жϲ��������Ƿ����
		if (formalParamNum > passedParamNum) {
			cerr << "Semantic Analysis ERROR!" << endl;
			cerr << "Less parameters given!" << endl;
			cerr << "FUNCTION " << calledFunc.varName << " expected " << formalParamNum <<
				" parameter(s), but " << passedParamNum << " given! Please check it!" << endl;
			return false;
		}

		//Ŀ�����
		string rs = targetCodeGenerator.getOperand(expression.idx);
		targetCodeGenerator.setInstruction({ "sw",rs,"($sp)","" });//func_param_x = T1  (T1 = a a = imm)  Ϊ��ʱ��������洢�ռ� �˴���ʾ����ʱ�����Ǳ����ú���ʵ��
		targetCodeGenerator.incStackPointer(1);//sp  = sp + 4 

		symbol.value = to_string(passedParamNum);//���ݵ�ʵ�θ���Ϊ��ȷ��
	}
	else if (prod.left == "CallFunction") {//CallFunction->( CallCheck ActualParamList )
		//Factor->id ( CallCheck ActualParamList )
		GrammarSymbol id = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 4];
		GrammarSymbol callCheck = grammarSymbolAttrStack[grammarSymbolAttrStack.size() - 1 - 2];

		symbol.name = prod.left;
		symbol.value = "";
		symbol.idx = callCheck.idx;

		//����һ������������Ԫʽ
		setQuad(Quad{ getNextQuad(),"call","-","-",id.value });
	}
	else if (prod.left == "FTYPE" && prod.right[0] == "CallFunction") {//FTYPE->CallFunction
		//�������Լ���
		GrammarSymbol callFunction = grammarSymbolAttrStack.back();

		symbol.name = prod.left;
		symbol.value = "CallFunction";//���� ����Ϊ��  ��ftype->$����
		symbol.idx = callFunction.idx;
	}
	else if (prod.left == "Program") {
		//����������
		//�ж��Ƿ���main����
		if (mainFuncLine == -1) {
			cerr << "Semantic Analysis ERROR!" << endl;
			cerr << "FUNCTION main was not defined! Please check it!" << endl;
			return false;
		}
		//���ɵ�0����Ԫʽ
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

	//����ʽ�Ҳ�����
	int len = prod.right[0] != "$" ? prod.right.size() : 0;
	//��Լ֮�� ����ջ����ı���Լ����ջ
	for (int i = 0; i < len; i++) {
		grammarSymbolAttrStack.pop_back();
	}
	//�ķ�����ջ����ѹ�����ķ����š�����
	grammarSymbolAttrStack.push_back(symbol);

	return true;
}