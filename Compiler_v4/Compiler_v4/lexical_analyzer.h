#pragma once
#include<iostream>
#include<set>
#include<string>
#include<vector>
#include<fstream>
#include<iomanip>
using namespace std;
enum class LEXICAL_TYPE {
    CINT = 0,//int�ͳ���
    CCHAR, //�ַ��ͳ���
    CFLOAT, //����������
    KEYWORD, //�ؼ���
    IDENTIFIER, //��ʶ��
    VARTYPE, //��������
    BORDER, //�߽����
    UNKNOWN, //δ֪����
    _EOF, //�ļ�������
    OPERATOR //������/�����
};

struct Word//����
{
    LEXICAL_TYPE type;//��������
    string name;//������grammar�е�����(�ķ�������)
    string value;//������Դ���еĵ��ʱ���(������ֵ�����߹ؼ��ֱ���)
    //���ں������õĲ���ʽ�õ�����ʾ������ȫ�ֱ��е����
    int functionIndex;
};

class LexicalAnalyzer {
private:
    //���������
    const set<string> OPERATORS = { "+", "-", "*", "/", "=", "==", ">", ">=", "<", "<=", "!=" };

    //�ؼ���
    const set<string> KEYWORDS = { "if", "else", "return", "while" };

    //��������
    const set<string> VAR_TYPES = { "int", "void", "char", "float" };

    //���
    const set<char> BORDERS = { '(', ')', '{', '}', ',', ';' };

    ifstream codeReader;
    ofstream resultPrinter;
    unsigned int lineCounter; //�ʷ���������������λ/�м���
    unsigned int stepCounter; //�ʷ������Ʋ���
    vector<unsigned char> buffer1;//������1
    vector<unsigned char> buffer2;//������2
    vector<unsigned char> currentBuffer;//��ǰ������
    int nextBuffer = 1;//��һ��Ӧ��ʹ�õĻ�����
    unsigned int bufferPoint = 0; //bufferָ���λ��
    unsigned int bufferSum = 0;//buffer ��ǰ�ַ�����
    bool backBuffer = false; //�Ƿ����buffer
    bool fileEnd = false; //�ļ�β


    bool isDot(const unsigned char ch);//�ж�С����
    bool isDigit(const unsigned char ch);//�ж� ����
    bool isLetter(const unsigned char ch);//�ж���ĸ
    bool isBlank(const unsigned char ch);//�ж��Ƿ��ǿո�
    bool isSingleCharOperator(const unsigned char ch);//�ж��Ƿ��ǵ����������
    bool isDoubleCharOperatorPre(const unsigned char ch);//�ж��Ƿ���˫���������ǰ��ĵ�һ���ַ�
    unsigned char getNextChar();//�õ��ַ�������һ���ַ�
    Word getBasicWord();//��ȡ�������� ��Ϊ�丳ֵ�������� �ؼ��ֵ�����



public:
    LexicalAnalyzer();
    ~LexicalAnalyzer();
    void packBuffer();
    bool openFile(const string&, const string&);
    unsigned char getBufferChar();//�õ�����������
    void backBufferPoint();
    Word getWord();//�õ�����
    void printLexicalResult(const Word&);
};
