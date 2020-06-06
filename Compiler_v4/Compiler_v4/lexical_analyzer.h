#pragma once
#include<iostream>
#include<set>
#include<string>
#include<vector>
#include<fstream>
#include<iomanip>
using namespace std;
enum class LEXICAL_TYPE {
    CINT = 0,//int型常数
    CCHAR, //字符型常数
    CFLOAT, //浮点数常数
    KEYWORD, //关键字
    IDENTIFIER, //标识符
    VARTYPE, //变量类型
    BORDER, //边界符号
    UNKNOWN, //未知符号
    _EOF, //文件结束符
    OPERATOR //操作符/运算符
};

struct Word//单词
{
    LEXICAL_TYPE type;//单词类型
    string name;//单词在grammar中的名称(文法符号名)
    string value;//待分析源码中的单词本身(常量的值，或者关键字本身)
    //仅在函数调用的产生式用到，表示函数在全局表中的序号
    int functionIndex;
};

class LexicalAnalyzer {
private:
    //所有运算符
    const set<string> OPERATORS = { "+", "-", "*", "/", "=", "==", ">", ">=", "<", "<=", "!=" };

    //关键字
    const set<string> KEYWORDS = { "if", "else", "return", "while" };

    //变量类型
    const set<string> VAR_TYPES = { "int", "void", "char", "float" };

    //界符
    const set<char> BORDERS = { '(', ')', '{', '}', ',', ';' };

    ifstream codeReader;
    ofstream resultPrinter;
    unsigned int lineCounter; //词法分析错误行数定位/行计数
    unsigned int stepCounter; //词法分析计步器
    vector<unsigned char> buffer1;//缓冲区1
    vector<unsigned char> buffer2;//缓冲区2
    vector<unsigned char> currentBuffer;//当前缓冲区
    int nextBuffer = 1;//下一次应该使用的缓冲区
    unsigned int bufferPoint = 0; //buffer指针的位置
    unsigned int bufferSum = 0;//buffer 当前字符总数
    bool backBuffer = false; //是否回退buffer
    bool fileEnd = false; //文件尾


    bool isDot(const unsigned char ch);//判断小数点
    bool isDigit(const unsigned char ch);//判断 数字
    bool isLetter(const unsigned char ch);//判断字母
    bool isBlank(const unsigned char ch);//判断是否是空格
    bool isSingleCharOperator(const unsigned char ch);//判断是否是单符号运算符
    bool isDoubleCharOperatorPre(const unsigned char ch);//判断是否是双符号运算符前面的第一个字符
    unsigned char getNextChar();//得到字符流中下一个字符
    Word getBasicWord();//获取基本单词 并为其赋值所属类型 关键字等属性



public:
    LexicalAnalyzer();
    ~LexicalAnalyzer();
    void packBuffer();
    bool openFile(const string&, const string&);
    unsigned char getBufferChar();//得到缓冲区符号
    void backBufferPoint();
    Word getWord();//得到单词
    void printLexicalResult(const Word&);
};
