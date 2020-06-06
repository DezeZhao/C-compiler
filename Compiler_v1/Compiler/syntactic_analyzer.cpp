#include"syntactic_analyzer.h"
using namespace std;

/*
	非终结符判断
*/
bool SyntacticAnalyzer::isNonTerminalSymbol(const string& symbol)
{
	if (symbol.length() == 0)
		return false;
	if (symbol[0] >= 'A' && symbol[0] <= 'Z')
		return true;
	return false;
}
/*
	分割产生式右部
*/
vector<string> SyntacticAnalyzer::split(const string& str, const string& delimiter) {
	vector<string> res;
	if ("" == str) return res;

	string str_ = str + delimiter;//在字符串末也添加一个分隔符 "A B "
	size_t pos = str_.find(delimiter);

	while (pos != str_.npos)//找不到就结束循环
	{
		res.push_back(str_.substr(0, pos));
		str_ = str_.substr(pos + 1);//将分割之后的字符串去掉
		pos = str_.find(delimiter);
	}
	return res;
}
/*
	从grammar.txt文件中读取产生式并存储
*/
bool SyntacticAnalyzer::getProd(const string & grammarFilename)
{
	ifstream grammarReader;

	grammarReader.open(grammarFilename);
	if (!grammarReader.is_open()) {
		cerr << "we meet an error when openning "<< grammarFilename<< "in the process of Syntactic Analysis. Please check if the file exists.";
	}
	cin.rdbuf(grammarReader.rdbuf());//重定向输入
	string grammarStr;//产生式字符串
	while (getline(cin,grammarStr)) {
		//getline(grammarReader, grammarStr);//读取一行产生式到grammarStr中
		if (grammarStr != "") {
			string left;//产生式左边
			vector<string>right;//产生式右边
			size_t leftPos = grammarStr.find_first_of("->");// unsigned int
			if (leftPos < 0) {
				cerr << "we meet an error when read production in grammar.txt beacuse of not finding symbol like \' --> \',please check if you have your prods normalized!";
				//关闭读取文件句柄
				grammarReader.close();
				return false;
			}
			left = grammarStr.substr(0, leftPos);
			nonterminalSymbols.insert(left);
			//不读"-->" grammar中存放-->右部产生式
			grammarStr = grammarStr.substr(leftPos + 2, string::npos);
			grammarStr += "|";//方便把最后一个候选式用统一的方式处理

			size_t  rightPos = grammarStr.find_first_of("|");

			//处理产生式右部
			while (rightPos != string::npos) {//没找到
				right.push_back(grammarStr.substr(0, rightPos));//将未分割的产生式直接存入right
				vector<string>splitProd = split(grammarStr.substr(0,rightPos), " ");//分割产生式
				prods.push_back({ left,splitProd });//将分割的产生式存入prods
				//添加终结符的文法符号集合到 grammar_symbolss中
				for (auto iter = splitProd.begin(); iter != splitProd.end(); iter++) {
					if (!isNonTerminalSymbol(*iter))
						terminalSymbols.insert(*iter);
				}
				grammarStr = grammarStr.substr(rightPos + 1, string::npos);//把处理过的候选式删除  最后一个处理完候选式为空
				rightPos = grammarStr.find_first_of("|");//最后一个候选式处理完就找不到|了 所以返回rightPos=string::npos
			}
			grammars.push_back({ left,right });
		}
	}
	terminalSymbols.insert("#");//#也加入终结符 后面LR1语法分析的时候会用到
	grammarReader.close();
	programStart = prods[0].left;//Program
	return true;
}
/*
	得到产生式串的first集合
*/
set<string> SyntacticAnalyzer::getProdFirstSet(const vector<string> & symbolStr) {

	set<string>prodFirstSet;//此处依靠firstSet求得产生式串的first集合 不需要循环  不需要重复检查
	for (auto iter = symbolStr.begin(); iter != symbolStr.end(); iter++) {
		if (terminalSymbols.find(*iter) != terminalSymbols.end()) {
			prodFirstSet.insert(*iter);//加入集合
			break;//终结符  跳出分析  此处#也会被加入
		}

		//非终结符 symbolStr -->XY
		//将first(X)中所有非epsilon元素加入First(XY)中
		for (auto iter1 = firstSet[*iter].begin(); iter1 != firstSet[*iter].end(); iter1++) {
			if (*iter1 != "$" && prodFirstSet.find(*iter) == prodFirstSet.end()) {
				prodFirstSet.insert(*iter1);
			}
		}
		//X的first 不含$ 则跳出分析
		if (firstSet[*iter].find("$") == firstSet[*iter].end()) {
			break;
		}

		//X  first含$

		//该产生式右部串的first集合必有$
		//存在Y0Y1...Yi-1Yi...Yk 每个Y的first集合都有$ && Yk为最后一个符号 则产生式右部串 first集合也有$
		if (iter + 1 == symbolStr.end() && prodFirstSet.find("$") == prodFirstSet.end()) {
			prodFirstSet.insert("$");
		}
	}
	return prodFirstSet;
}
/*
	求出非终结符的first集合
*/
void SyntacticAnalyzer::getFirstSet()
{
	//用[]操作符可以直接创建map的key 
	//if X is in VN 开始符号是非终结符
	while (true) {
		bool change = false;
		for (auto iterKey = prods.begin(); iterKey != prods.end(); iterKey++) {//每个产生式
			auto key = (*iterKey).left;//string
			auto vals = (*iterKey).right;//vector<string>
			for (auto iterVals = vals.begin(); iterVals != vals.end(); iterVals++) {

				// 存在X-- > a... | X->epsilon的情况 将其加入first集合
				// 存在X --> Y0Y1Y2...Yi-1Yi 此时Yi为终结符Y0...Yi-1 First集合含有$ 
				if (terminalSymbols.find(*iterVals) != terminalSymbols.end()) {
					if (firstSet[key].find(*iterVals) == firstSet[key].end()) {//并且first集合中没有该元素
						firstSet[key].insert(*iterVals);
						change = true;
					}
					break;//此产生式分析结束 遇到终结符就必须结束  没办法向下分析了
				}

				//存在X-->Y...,将first(Y)中所有非epsilon元素加入First(X)中
				//此处不需要判断 产生式右部肯定是非终结符
				for (auto iter = firstSet[*iterVals].begin(); iter != firstSet[*iterVals].end(); iter++) {
					if (firstSet[key].find(*iter) == firstSet[key].end() && (*iter) != "$") {//first集合中没有该元素 !=$
						firstSet[key].insert(*iter);
						change = true;
					}
				}

				//再需要判断此处的Y的first集合是不是含有$ 若没有 此产生式分析结束
				//(因为key不会再推导出Y的first集合之外的符号了)
				if (firstSet[*iterVals].find("$") == firstSet[*iterVals].end()) {
					break;
				}
				//该符号的first集合必有$

				//存在X-->Y0Y1...Yi-1Yi...Yk 每个Y的first集合都有$ && Yk为最后一个符号 则key first集合也有$
				if (iterVals + 1 == vals.end() && firstSet[key].find("$") == firstSet[key].end()) {//first集合中没有该元素$
					firstSet[key].insert("$");
					change = true;
				}
			}
		}
		if (change == false)
			break;
	}
	return;
}
/*
	求出非终结符的follow集合
*/
void SyntacticAnalyzer::getFollowSet() {
	//初始化将#放入Follow(S)中
	followSet[prods[0].left].insert("#");
	//生成follow集合
	while (true) {
		bool change = false;
		for (auto iterKey = prods.begin(); iterKey != prods.end(); iterKey++) {//每个产生式
			auto key = (*iterKey).left;//string
			auto vals = (*iterKey).right;//vector
			for (auto iterVals = vals.begin(); iterVals != vals.end(); iterVals++) {//产生式右部
				//如果是终结符跳过；
				if (terminalSymbols.find(*iterVals) != terminalSymbols.end()) {
					continue;
				}

				//该字符必然是终结符
				//即出现 X-->aBb (a,b分别是字符串)  此时把First(b)/$加入Follow(B)
				set<string> ss = getProdFirstSet(vector<string>(iterVals + 1, vals.end()));
				//相当于求字符串b的first集合
				for (auto iter = ss.begin(); iter != ss.end(); iter++) {
					if (followSet[*iterVals].find(*iter) == followSet[*iterVals].end() && *iter != "$") {
						followSet[*iterVals].insert(*iter);
						change = true;
					}
				}
				//若 X-->aBb b==>$ ||X-->aB  则将Follow(X)加入Follow(B)
				if (ss.find("$") != ss.end() || ss.empty()) {//first(b)中含有$
					for (auto iter = followSet[key].begin(); iter != followSet[key].end(); iter++) {
						if (followSet[*iterVals].find(*iter) == followSet[*iterVals].end()) {
							followSet[*iterVals].insert(*iter);
							change = true;
						}
					}
				}
			}
		}
		if (change == false) {
			break;
		}
	}
	return;
}
/*
	得到拓广文法 ExtendGrammar
*/
void SyntacticAnalyzer::getExtendGrammar() {
	//生成拓广文法
	string left = "Start";
	vector<string> right;
	right.push_back(prods[0].left);
	prods.insert(prods.begin(), { left,right });//0  Start-->Program
}
/*
	得到所有LR(0)项目  用prods中的所有单个产生式来构造
	存入 lr0Items 向量中
*/
void SyntacticAnalyzer::getLR0Items() {
	int id = 0;
	for (auto iterKey = prods.begin(); iterKey != prods.end(); iterKey++) {
		auto vals = iterKey->right;//vector
		size_t prodPos = iterKey - prods.begin();//0 1 2 3...
		size_t pointPos = 0;
		int epsilon_exists = 0;
		size_t i = 0;
		for (i = 0; i <  vals.size();i++) {
			pointPos = i;
			lr0Items.push_back({ prodPos,pointPos});
			//此处用位置索引相对更好 若是直接将"·"加入right，格式的控制比较困难
			if (vals[i] == "$") {//A->$ ==>A->·
				epsilon_exists = 1;
				break; //此时i= 0;
			}
		}
		if (!epsilon_exists) {
			pointPos = vals.size();
			lr0Items.push_back({ prodPos,pointPos });
		}
	}
}
/*
	由一个LR1项目得到一个项目集闭包I0
*/
void SyntacticAnalyzer::getLR1ItemClosure0() {
	
	stack<LR_1_Item>lr1ItemStack;//用该栈来分析闭包
	//set<LR_1_Item>lr1ItemClosure;//最终将该闭包集合返回
	lr1ItemStack.push(lr1Item0);
	
	size_t prodPos, pointPos;
	vector<string> first;
	
	//用循环来判断直到栈空
	while (!lr1ItemStack.empty()) {
		first.clear();//清空first集合

		auto temp = lr1ItemStack.top();//拿到栈顶元素
		lr1ItemStack.pop();//出栈
		lr1ItemClosure0.insert(temp);//存进项目集闭包

		if (prods[temp.prodPos].right[0] == "$") {
			continue;//不分析类似A-->·
		}

		//开始分析
		prodPos = temp.prodPos;
		pointPos = temp.pointPos;
		auto right = prods[prodPos].right;

		//圆点在right的最后 为规约串 只需将其加入要返回的项目集闭包即可 无需分析 A-->xB·   A-->XXa· .....
		if (temp.pointPos == right.size()) {
			continue;
		}
		//圆点不在right最后&&非规约串
		//终结符 A-->·a  A-->B·aXXX .....此处不需要进行分析  直接将其加入项目集闭包
		if (terminalSymbols.find(right[temp.pointPos]) != terminalSymbols.end())
			continue;
		//非终结符
		//将当前符号的后一个开始和展望串拼接 求其first集合  有 A-->a·B,xx   或者A-->a·Bb,xx .....
		first.insert(first.end(), right.begin() + pointPos + 1, right.end());//有可能为空
		size_t len =first.end()-first.begin();
		set<string> lookaheadStr;
		for (auto iter = temp.lookaheadStr.begin(); iter != temp.lookaheadStr.end(); iter++) {
			//一定不为空（可能只有#）将lookaheadStr中每个元素作为单独的一个字符加入 并求产生式串的first集合
			first.insert(first.begin() + len, *iter);//将v插入first之后
			set<string>s = getProdFirstSet(first);//得到展望串,#的first集合相当于终结符的first集合,为其自身
			lookaheadStr.insert(s.begin(), s.end());//最后再合并到一个集合里
		}

		//再求对应的B->·XXXXX.. 在LR0中寻找对应的产生式左部为B && pointPos=0;
		for (auto iter = lr0Items.begin(); iter != lr0Items.end(); iter++) {
			if (iter->pointPos == 0 && prods[iter->prodPos].left == right[pointPos]) {
				if (lr1ItemClosure0.find({ iter->prodPos,iter->pointPos,lookaheadStr }) == lr1ItemClosure0.end())
					/*集合中不存在该LR1项目 才能将该项目压栈！！！！！！！(very 重要！！！)
					不能被set的不重复性给欺骗！！！！虽然set元素不重复，但是此处要是不判断
					就有可能将相同的LR1项目压栈，此处的LR1项目是符合条件的，可以进行分析的，所以每回
					它出栈就能到这一步，导致程序陷入分析循环中！*/
					lr1ItemStack.push(LR_1_Item{ iter->prodPos,iter->pointPos,lookaheadStr });
			}
		}
	}
}
/*
	go函数 用于生成 DFA 返回一个项目集闭包
	go(I,X)=Closure(J)项目集I输入文法符号X之后得到的新闭包
*/
set<LR_1_Item> SyntacticAnalyzer::go(const set<LR_1_Item>& lr1Items)
{
	stack<LR_1_Item>lr1ItemsStack;//用该栈来分析闭包
	set<LR_1_Item>lr1ItemsClosure;//最终将该闭包集合返回
	for (auto iter = lr1Items.begin(); iter != lr1Items.end(); iter++) {
		lr1ItemsStack.push(*iter);
	}
	size_t prodPos, pointPos;
	vector<string> first;

	//用循环来判断直到栈空
	while (!lr1ItemsStack.empty()) {
		first.clear();//清空first集合

		auto temp = lr1ItemsStack.top();//拿到栈顶元素
		lr1ItemsStack.pop();//出栈
		lr1ItemsClosure.insert(temp);//存进项目集闭包

		if (prods[temp.prodPos].right[0] == "$") {
			continue;//不分析类似A-->·
		}

		//开始分析
		prodPos = temp.prodPos;
		pointPos = temp.pointPos;
		auto right = prods[prodPos].right;

		//圆点在right的最后 为规约串 只需将其加入要返回的项目集闭包即可 无需分析 A-->xB·   A-->XXa· .....
		if (temp.pointPos == right.size()) {
			continue;
		}
		//圆点不在right最后&&非规约串
		//终结符 A-->·a  A-->B·aXXX .....此处不需要进行分析  直接将其加入项目集闭包
		if (terminalSymbols.find(right[temp.pointPos]) != terminalSymbols.end())
			continue;
		//非终结符
		//将当前符号的后一个开始和展望串拼接 求其first集合  有 A-->a·B,xx   或者A-->a·Bb,xx .....
		first.insert(first.end(), right.begin() + pointPos + 1, right.end());//有可能为空
		size_t len = first.end() - first.begin();
		set<string> lookaheadStr;
		for (auto iter = temp.lookaheadStr.begin(); iter != temp.lookaheadStr.end(); iter++) {
			//一定不为空（可能只有#）将lookaheadStr中每个元素作为单独的一个字符加入 并求产生式串的first集合
			first.insert(first.begin() + len, *iter);//将v插入first之后
			set<string>s = getProdFirstSet(first);//得到展望串,#的first集合相当于终结符的first集合,为其自身
			lookaheadStr.insert(s.begin(), s.end());//最后再合并到一个集合里
		}

		//再求对应的B->·XXXXX.. 在LR0中寻找对应的产生式左部为B && pointPos=0;
		for (auto iter = lr0Items.begin(); iter != lr0Items.end(); iter++) {
			if (iter->pointPos == 0 && prods[iter->prodPos].left == right[pointPos]) {
				if (lr1ItemsClosure.find({ iter->prodPos,iter->pointPos,lookaheadStr }) == lr1ItemsClosure.end())
					lr1ItemsStack.push(LR_1_Item{ iter->prodPos,iter->pointPos,lookaheadStr });
			}
		}
	}
	return lr1ItemsClosure;
}
/*
	构造项目集规范族  也就是项目集的闭包的集合
	构成识别一个文法活前缀的DFA的项目集（状态）的全体称为文法的LR(1)项目集规范族。
*/
void SyntacticAnalyzer::getLR1ItemsNormalFamily() {

	queue<set<LR_1_Item>> lr1ItemsQueue;
	lr1ItemsQueue.push(lr1ItemClosure0);//初始项目集闭包入队
	lr1ItemsNormalFamily.push_back(lr1ItemClosure0);//保存到项目集规范族

	//开始利用go函数生成项目集规范族
	while (!lr1ItemsQueue.empty()) {//队列非空
		auto temp = lr1ItemsQueue.front();//暂存
		lr1ItemsQueue.pop();

		auto currIter = find(lr1ItemsNormalFamily.begin(), lr1ItemsNormalFamily.end(), temp);
		int currState = currIter-lr1ItemsNormalFamily.begin() ;//得到当前项目集闭包在项目集规范族中的位置 即状态序号
		//遍历初始闭包中的每个项目
		for (auto iter1 = temp.begin(); iter1 != temp.end(); iter1++) {
			//当前项目的圆点的位置及对应的prods的下标
			auto prodPos = iter1->prodPos;
			auto pointPos = iter1->pointPos;
			auto lookaheadStr = iter1->lookaheadStr;

			//A-->·直接跳过 不分析  不单独作为一个项目集
			if (pointPos == 0 && prods[prodPos].right[0] == "$")
				continue;

			//分析当前项目
			
			//1.规约的情况 + accept情况
			/*
				情形 1. 若项目[A→α•， a]属于Ik，则置ACTION[k, a]为
				"rj"；其中假定A→α为文法G′的第j个产生式
				情形 2. 若项目[S′→S•, #]属于Ik，则置ACTION[k, #]为
				"acc";
			*/
			if (pointPos == prods[prodPos].right.size()) {//A-->XXa·||A-->XX·..
				//此处直接构造LR1分析表即可
				if (programStart == prods[prodPos].right[0]) {//情形 2
					LR1AnalysisTable.insert({ {currState,"#"} ,{Action::ACCEPT,-1} });//-1表示没有nextState
				}
				else {//情形 1
					for (auto iter2 = lookaheadStr.begin(); iter2 != lookaheadStr.end(); iter2++) {//遍历展望串
						auto nextState = find(prods.begin(), prods.end(), prods[prodPos]) - prods.begin();//得到该项目对应的产生式编号
						LR1AnalysisTable.insert({ {currState,*iter2},{Action::REDUCE,nextState} });//action goto表构造(LR1分析表)
					}
				}
			}
			else {//2. 移进的情况
				/*
					情形1. 若项目[A→α•aβ, b]属于Ik且GO(Ik, a)＝ Ij， a为
					终结符，则置ACTION[k, a]为 "sj"。
					情形2. 若GO(Ik， A)＝ Ij，则置GOTO[k, A]=j
				*/
				set<LR_1_Item>tempLR1Items;//存放具有相同下一个输入文法符号的LR1项目
				/*此处需要注意  go函数是将一个项目集作为状态 某个文法符号作为输入符号
				而且还要注意  一个项目集中可能存在多个项目 它们下一个输入符号相同！！！
				所以统一存入集合传给go函数*/
				for (auto iter3 = temp.begin(); iter3 != temp.end(); iter3++) {
					//防止vector越界 先判断prods[iter3->prodPos].right是否有iter3->pointPos下标
					if (prods[iter3->prodPos].right.size() > iter3->pointPos) {
						//cout << prods[14].right[1] << endl;
						if (prods[iter3->prodPos].right[iter3->pointPos] == prods[prodPos].right[pointPos]) {//下一个输入文法符号相同
							tempLR1Items.insert({ iter3->prodPos,iter3->pointPos + 1,iter3->lookaheadStr });//pointPos+1
						}
					}
				}

				//求tempLR1Items的项目集闭包
				set<LR_1_Item>tempLR1ItemsClosure = go(tempLR1Items);

				//判断该项目集闭包是否在项目集规范族中
				if (find(lr1ItemsNormalFamily.begin(), lr1ItemsNormalFamily.end(), tempLR1ItemsClosure) == lr1ItemsNormalFamily.end()) {//不存在
					lr1ItemsNormalFamily.push_back(tempLR1ItemsClosure);//保存到项目集规范族
					lr1ItemsQueue.push(tempLR1ItemsClosure);//插入队列
				}

				//存在不存在都要构造LR1分析表
				auto nextIter = find(lr1ItemsNormalFamily.begin(), lr1ItemsNormalFamily.end(), tempLR1ItemsClosure);
				int nextState = nextIter - lr1ItemsNormalFamily.begin();//在项目集规范族中找tempLR1ItemsClosure的状态编号
				StateSymbol ss = { currState,prods[prodPos].right[pointPos] };

				if (LR1AnalysisTable.find(ss) == LR1AnalysisTable.end()) {//分析表没有该键值对
					//判断移进的是终结符还是非终结符 只需要对主循环里面的项目进行符号判定即可
					if (terminalSymbols.find(prods[prodPos].right[pointPos]) != terminalSymbols.end()) {//终结符 情形1
						LR1AnalysisTable.insert({ ss,{ Action::SHIFT,nextState } });
					}
					else {//非终结符 情形2
						LR1AnalysisTable.insert({ ss,{Action::NOACTION,nextState} });
					}
				}
			}
		}
	}
}
/*
	打印一个LR0项目
*/
void SyntacticAnalyzer::printLR0Item(const LR_0_Item& lr0Item) {
	cout << prods[lr0Item.prodPos].left << "->";//输出左部
	auto right = prods[lr0Item.prodPos].right;
	for (auto iter1 = right.begin(); iter1 != right.end(); iter1++) {
		if (*iter1 == "$") {
			cout << "·";
			break;
		}
		if (lr0Item.pointPos == (iter1 - right.begin())) {
			cout << "· ";
			if (iter1 + 1 == right.end())
				cout << *iter1;
			else
				cout << *iter1 << " ";
		}
		else {
			if (iter1 + 1 == right.end())
				cout << *iter1;
			else
				cout << *iter1 << " ";
		}
	}
	if (lr0Item.pointPos == right.size())
		cout << "·";
}
/*
	打印所有LR0项目
*/
void SyntacticAnalyzer::printLR0Items() {
	for (auto iter = lr0Items.begin(); iter != lr0Items.end(); iter++) {
		printLR0Item(*iter);
		cout << endl;
	}
}
/*
	打印项目集规范族
*/
void SyntacticAnalyzer::printLR1ItemsNormalFamily() {

	int id = 0;
	for (auto iter = lr1ItemsNormalFamily.begin(); iter != lr1ItemsNormalFamily.end(); iter++) {
		++id;
		cout << "===========================项目集I"<<id<<"==============================" << endl;
		for (auto iter1 = iter->begin(); iter1 != iter->end(); iter1++) {
			printLR0Item({ iter1->prodPos,iter1->pointPos });//打印LR0项目
			cout << " ,[";
			print(iter1->lookaheadStr);//打印展望串
			cout << "]" << endl;
		}
		cout <<endl<<endl<< endl;
	}
}
/*
	打印文法符号—— 终结符 非终结符
*/
void SyntacticAnalyzer::printGrammarSymbols(){
	cout << "==========================Terminal Symbols=================================" << endl;
	for (auto iter = terminalSymbols.begin(); iter != terminalSymbols.end(); iter++) {
		cout << *iter << endl;
	}
	cout << "==========================Nonterminal Symbols==============================" << endl;
	for (auto iter = nonterminalSymbols.begin(); iter != nonterminalSymbols.end(); iter++) {
		cout << *iter << endl;
	}
}
/*
	打印单个产生式集合
*/
void SyntacticAnalyzer::printProductions() {
	print(prods);
}
/*
	打印ActionGoto表 LR1分析表 csv文件
*/
void SyntacticAnalyzer::printLR1AnalysisTabel() {
	ofstream toCSV;
	toCSV.open("./gen_data/LR1AnaLysisTable.csv", ios::out | ios::trunc);//清空已有的文件
	cout.rdbuf(toCSV.rdbuf());//重定向
	if (toCSV.fail()) {
		cerr << "fail to open file LR1AnaLysisTable.csv!";
	}
	//第一行的文法符号已经写入
	cout << "State "<<",";
	for (auto iter = terminalSymbols.begin(); iter != terminalSymbols.end(); iter++) {
		if (*iter != ",")
			cout << *iter << ",";
		else
			cout << "，" << ",";
	}
	for (auto iter = nonterminalSymbols.begin(); iter != nonterminalSymbols.end(); iter++) {
		if (*iter != ",")
			cout << *iter << ",";
		else
			cout << "，" << ",";
	}
	cout << endl;//换行

	vector<string>symbols;
	//将终结符和非终结符集合里面的文法符号存进vector
	for (auto iter = terminalSymbols.begin(); iter != terminalSymbols.end(); iter++) {
		symbols.push_back(*iter);
	}
	for (auto iter = nonterminalSymbols.begin(); iter != nonterminalSymbols.end(); iter++) {
		symbols.push_back(*iter);
	}

	//第一列中逐状态填写每行对应的动作和下一个状态
	for (auto i = 0; i < lr1ItemsNormalFamily.size(); i++) {//状态0,1,2,3,4...
		cout << i << ",";
		for (auto j = 0; j < symbols.size(); j++) {
			if (LR1AnalysisTable.find({ i,symbols[j] }) != LR1AnalysisTable.end()) {//LR1分析表中存在该动作和状态的映射
				if (j < terminalSymbols.size()) {//终结符 ACTION
					if (LR1AnalysisTable[{i, symbols[j]}].action == Action::REDUCE)
						cout << "r";
					else if (LR1AnalysisTable[{i, symbols[j]}].action == Action::SHIFT)
						cout << "s";
					else if (LR1AnalysisTable[{i, symbols[j]}].action == Action::ACCEPT)
						cout << "acc";
					else if (LR1AnalysisTable[{i, symbols[j]}].action == Action::NOACTION) {
						cout << " ";
					}
					if (LR1AnalysisTable[{i, symbols[j]}].nextState != -1)
						cout << LR1AnalysisTable[{i, symbols[j]}].nextState << ",";
				}
				else {//非终结符 GOTO
					cout << LR1AnalysisTable[{i, symbols[j]}].nextState << ",";
				}
			}
			else {
				cout << " " << ",";
			}
		}
		cout << endl;
	}
}

void SyntacticAnalyzer::funcPrint() {
	ofstream out;
	out.open("./gen_data/FirstSet.txt", ios::out);
	if (out.fail()) {
		cerr << "fail to open file FirstSet.txt!";
	}
	cout.rdbuf(out.rdbuf());
	print(firstSet);//打印first集合

	out.close();

	out.open("./gen_data/FollowSet.txt", ios::out);
	if (out.fail()) {
		cerr << "fail to open file FollowSet.txt!";
	}
	cout.rdbuf(out.rdbuf());
	print(followSet);//打印follow集合

	out.close();


	out.open("./gen_data/LR0Items.txt", ios::out);
	if (out.fail()) {
		cerr << "fail to open file LR0Items.txt!";
	}
	cout.rdbuf(out.rdbuf());
	printLR0Items();//打印LR0项目

	out.close();

	out.open("./gen_data/LR1ItemsNormalFamily.txt");
	if (out.fail()) {
		cerr << "fail to open LR1ItemsNormalFamily.txt!";
	}
	cout.rdbuf(out.rdbuf());
	printLR1ItemsNormalFamily();//打印项目集规范族

	out.close();
	out.open("./gen_data/GrammarSymbols.txt");
	if (out.fail()) {
		cerr << "fail to open GrammarSymbols.txt!";
	}
	cout.rdbuf(out.rdbuf());
	printGrammarSymbols();//打印文法符号

	out.close();
	out.open("./gen_data/Productions.txt");
	if (out.fail()) {
		cerr << "fail to open Productions.txt!";
	}
	cout.rdbuf(out.rdbuf());
	printProductions();//打印单个产生式集合
	printLR1AnalysisTabel();
}

void SyntacticAnalyzer::print(const vector<Production> &vec)
{
	for (auto iter = vec.begin(); iter != vec.end(); iter++) {
		cout << (*iter).left<< "->";
		for (auto iter1 = (*iter).right.begin(); iter1 != (*iter).right.end(); iter1++) {
			if (iter1 + 1 == (*iter).right.end()) {
				cout << *iter1 << endl;
			}
			else
				cout << *iter1 << " ";
		}
	}
}

void SyntacticAnalyzer::print(const set<string>& vec) {
	for (auto iter = vec.begin(); iter != vec.end(); iter++) {
		if (*iter == *vec.rbegin())
			cout << *iter;
		else 
			cout << *iter << " ";
	}
}

void SyntacticAnalyzer::print(const vector<string>& vec) {
	for (auto iter = vec.begin(); iter != vec.end(); iter++) {
		if (iter + 1 == vec.end())
			cout << *iter << endl;
		else
			cout << *iter << " ";
	}
}

void SyntacticAnalyzer::print(const map<string, set<string>> & m) {
	for (auto iter = m.begin(); iter != m.end(); iter++) {
		cout << (*iter).first << ":[";
		print((*iter).second);
		cout << "]" << endl;;
	}
}

void SyntacticAnalyzer::test()
{
	getProd("./raw_data/grammar_.txt");//得到单个产生式集合
	getExtendGrammar();//生成拓广文法
	getFirstSet();//得到First集合
	getFollowSet();//得到Follow集合
	getLR0Items(); //得到所有LR0项目
	lr1Item0 = LR_1_Item{ lr0Items[0].prodPos,lr0Items[0].pointPos,set<string>{"#"} };
	getLR1ItemClosure0();//得到项目集闭包0
	getLR1ItemsNormalFamily();//求项目集规范族
	funcPrint();
}


