#include"lexical_analyzer.h"
const char* Type[] = { "INT","CHAR","FLOAT","KEYWARD", "IDENTIFIER", "VARTYPE", "BORDER", "UNKNOWN", "_EOF", "OPERATOR" };

LexicalAnalyzer::LexicalAnalyzer()
{
    lineCounter = 1;
    stepCounter = 1;
    //print_detail_ = false;
}

LexicalAnalyzer::~LexicalAnalyzer()
{
    /*
        关闭文件流
    */
    if (codeReader.is_open())
        codeReader.close();

    if (resultPrinter.is_open())
        resultPrinter.close();
}

bool LexicalAnalyzer::openFile(const string& codeFilename, const string& resultFilename) {
    resultPrinter.open(resultFilename);
    if (!resultPrinter.is_open()) {
        cerr << "we meet an error when openning " << resultFilename << " in the Lexical Analysis.";
        return false;
    }
    resultPrinter << "step" << "," << "type" << "," << "value" << "," << "line" << endl;;

    codeReader.open(codeFilename);
    if (!codeReader.is_open()) {
        cerr << "we meet an error when openning " << codeFilename <<
            " in the process of Lexical Analysis. Please check if the file exists.";
        return false;
    }
    return true;
}

bool LexicalAnalyzer::isDot(const unsigned char ch) {
    if (ch == '.')
        return true;
    else
        return false;
}

bool LexicalAnalyzer::isDigit(const unsigned char ch) {
    if (ch >= '0' && ch <= '9')
        return true;
    else
        return false;
}

bool LexicalAnalyzer::isLetter(const unsigned char ch) {
    if (ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z' || '_' == ch) //包含下划线
        return true;
    else
        return false;
}

bool LexicalAnalyzer::isBlank(const unsigned char ch) {
    if (' ' == ch || '\n' == ch || '\t' == ch || 255 == ch)
        return true;
    else
        return false;
}

bool LexicalAnalyzer::isSingleCharOperator(const unsigned char ch) {
    if (ch == '+' || ch == '-' || ch == '*' || ch == '/')
        return true;
    else
        return false;
}

bool LexicalAnalyzer::isDoubleCharOperatorPre(const unsigned char ch) {
    if (ch == '=' || ch == '<' || ch == '>' || ch == '!')
        return true;
    else
        return false;
}

unsigned char LexicalAnalyzer::getNextChar() {
    unsigned char ch = codeReader.get();
    if ('\n' == ch) {
        lineCounter++;//一行读取完毕 行数加1
        //cout << lineCounter << endl;
    }
    return ch;
}

void LexicalAnalyzer::packBuffer() {
    int sum = 0;
    unsigned char ch;
    unsigned char nextChar;
    bool commentMode = false;
    bool stringMode = false;
    if (nextBuffer == 1) {
        buffer1.clear();
        buffer2 = currentBuffer;
        currentBuffer = buffer1;
        nextBuffer = 2;//下一次使用buffer2
    }
    else {
        buffer2.clear();
        buffer1 = currentBuffer;
        currentBuffer = buffer2;
        nextBuffer = 1;//下一次使用buffer1
    }
    while (sum < 120 && !codeReader.eof()) {//填满一个buffer
        ch = getNextChar();
        if (ch == '/') {//处理注释

            commentMode = true;
            nextChar = codeReader.peek();//输入流指针不向后移动只是观测 get则会移动 
            if (nextChar == '/') {//双斜杠注释

                while (getNextChar() != '\n' && !codeReader.eof());//到回车为止
                commentMode = false;
            }
            else if (nextChar == '*') {// /**/注释

                while (!codeReader.eof()) {

                    while (getNextChar() != '*' && !codeReader.eof());//到*为止
                    //UNKNOWN
                    if (codeReader.eof()) {//非正常完结的注释

                        cout << "ERROR End" << endl;
                    }

                    //  /**/ 注释结束
                    if (getNextChar() == '/') {
                        commentMode = false;
                        break;
                    }
                }
            }
            else {//这是个除号

                currentBuffer.push_back(ch);
                sum++;
            }
        }
        else {//非注释
            currentBuffer.push_back(ch);
            sum++;
            if (ch == ' ') {//去空格
                //到下一个不是空格为止
                while (codeReader.peek() == ' ' && !codeReader.eof()) {
                    getNextChar();
                }
            }
        }
    }
    if (codeReader.eof()) {
        fileEnd = true;
    }
    bufferSum = sum;
}

unsigned char LexicalAnalyzer::getBufferChar()
{
    if (bufferPoint >= bufferSum && fileEnd == true) {//假如EOF了就返回上一个而不是现在的
        return currentBuffer[bufferPoint - 1];
    }
    unsigned char ch;
    if (bufferPoint == 120) {
        if (backBuffer == true) {//假如是从上一个缓冲区退回来的,则此时下一个缓冲区已经装填好了
            backBuffer = false;
            if (nextBuffer == 1) {
                currentBuffer = buffer1;
                bufferPoint = 0;
            }
            else {
                currentBuffer = buffer2;
                bufferPoint = 0;
            }
        }
        else {
            packBuffer();
            bufferPoint = 0;
        }
    }
    ch = currentBuffer[bufferPoint];
    bufferPoint++;
    return ch;
}

void LexicalAnalyzer::backBufferPoint() {
    if (bufferPoint != 0) {
        bufferPoint--;
    }
    else {//假如是零则要退到上一个缓冲区
        if (nextBuffer == 1) {
            currentBuffer = buffer1;
            nextBuffer = 2;
            bufferPoint = 119;
            backBuffer = true;
        }
        else {
            currentBuffer = buffer2;
            nextBuffer = 1;
            bufferPoint = 119;
            backBuffer = true;
        }
    }
}

Word LexicalAnalyzer::getBasicWord() {
    unsigned char ch;
    string str_token;
    Word word;

    while (!(bufferPoint >= bufferSum && fileEnd == true))
    {
        int flag = 0;
        ch = getBufferChar();
        if (isDigit(ch))
        {
            str_token += ch;
            while (!(bufferPoint >= bufferSum && fileEnd == true)) {
                unsigned char nextChar = getBufferChar();
                if (isDigit(nextChar)) //下一个是数字
                {
                    str_token += nextChar;
                }
                else //下一个不是数字
                {
                    if (isDot(nextChar)) //是小数，继续循环判断后面的数字
                    {
                        str_token += nextChar;
                        flag = 1; //是小数
                        continue;
                    }
                    else //不是小数,是int型数
                    {
                        backBufferPoint();//输入文件指针回退,因为多读了nextChar
                        word.value = str_token;
                        if (flag == 0)
                            word.type = LEXICAL_TYPE::CINT; //int数
                        else if (flag == 1)
                            word.type = LEXICAL_TYPE::CFLOAT; //float数
                        return word;
                    }
                }
            }
        }
        else if (isLetter(ch))
        {
            str_token += ch;
            while (!(bufferPoint == bufferSum && fileEnd == true)) {
                unsigned char nextChar = getBufferChar();
                if ((!isDigit(nextChar)) && (!isLetter(nextChar))) //下一个不是字母或数字，需要退回
                {
                    backBufferPoint();
                    //判断是否为关键字、变量类型
                    if (KEYWORDS.find(str_token) != KEYWORDS.end()) //在关键字中
                        word.type = LEXICAL_TYPE::KEYWORD;
                    else if (VAR_TYPES.find(str_token) != VAR_TYPES.end()) //在变量类型中
                        word.type = LEXICAL_TYPE::VARTYPE;
                    else //标识符
                        word.type = LEXICAL_TYPE::IDENTIFIER;

                    word.value = str_token;
                    return word;
                }
                else  //下一个还是字母或数字，继续循环
                    str_token += nextChar;
            }
        }
        else if (BORDERS.find(ch) != BORDERS.end()) {
            word.type = LEXICAL_TYPE::BORDER; //边界
            word.value = ch;
            return word;
        }
        else if (isSingleCharOperator(ch))//单符号运算符
        {
            word.type = LEXICAL_TYPE::OPERATOR; //操作符
            word.value = ch;
            return word;
        }
        else if (isDoubleCharOperatorPre(ch))//> = < !判断是否为双符号运算符
        {
            str_token += ch;
            unsigned char nextChar = getBufferChar();

            if ('=' == nextChar) //确定是双符号运算符 == >= <= !=
            {
                str_token += nextChar;
                word.type = LEXICAL_TYPE::OPERATOR; //操作符
                word.value = str_token;
                return word;
            }
            else
            {
                backBufferPoint();//输入文件指针回退,因为多读了nextChar
                if (ch != '!')//是个单符号操作符 不是!= 那必定为>/</ =
                {
                    word.type = LEXICAL_TYPE::OPERATOR; //操作符
                    word.value = str_token;
                    return word;
                }
                else// ! 未定义的符号
                {
                    word.type = LEXICAL_TYPE::UNKNOWN; //未知操作符
                    word.value = str_token;
                    return word;
                }
            }
        }
        else if (isBlank(ch))
        {
            continue;
        }
        else if (ch == '\'') //开头是字符型标志\'
        {
            str_token += ch;
            while (!(bufferPoint >= bufferSum && fileEnd == true))
            {
                unsigned char nextChar = getBufferChar();
                if (isDigit(nextChar) || isLetter(nextChar) || isBlank(nextChar)) //数字，字母，' ',\n,\t,255
                    str_token += nextChar;
                else if (nextChar == '\'') {//\'(字符结束标志)
                    str_token += nextChar;
                    word.value = str_token;
                    word.type = LEXICAL_TYPE::CCHAR; //char型常量
                    return word;
                }
            }
        }
        else
        {
            word.type = LEXICAL_TYPE::UNKNOWN;//未知
            word.value = ch;
            /*cout << ch << "***********************" << endl;*/
            //word.value += "," + std::to_string(lineCounter);//出错 行计数
            return word;
        }
    }
    word.type = LEXICAL_TYPE::_EOF;
    word.value = "#";
    return word;
}

Word LexicalAnalyzer::getWord() {
    Word word = getBasicWord();
    string wordStr;
    //此处的wordStr是要和语法分析的文法中终结符相同
    if (word.type == LEXICAL_TYPE::_EOF)
        wordStr = "#";
    else if (word.type == LEXICAL_TYPE::CINT)
        wordStr = "const_int";
    else if (word.type == LEXICAL_TYPE::CCHAR)
        wordStr = "const_char";
    else if (word.type == LEXICAL_TYPE::CFLOAT)
        wordStr = "const_float";
    else if (word.type == LEXICAL_TYPE::KEYWORD //KEYWORDS = { "if", "else", "return", "while" };
        || word.type == LEXICAL_TYPE::VARTYPE // VAR_TYPES = { "int", "void", "char", "float" };
        || word.type == LEXICAL_TYPE::BORDER  //BORDERS = { '(', ')', '{', '}', ',', ';' };
        || word.type == LEXICAL_TYPE::OPERATOR) //OPERATORS = { "+", "-", "*", "/", "=", "==", ">", ">=", "<", "<=", "!=" };
        wordStr = word.value;
    else if (word.type == LEXICAL_TYPE::IDENTIFIER)
        wordStr = "id";
    else { ; }

    word.wordStr = wordStr;
    printLexicalResult(word);
    return word;
}

void LexicalAnalyzer::printLexicalResult(const Word& word) {
    if (word.type == LEXICAL_TYPE::_EOF)
        return;
    if (word.value == ",")
        resultPrinter << stepCounter << "," << Type[int(word.type)] << "," << "，" << endl;
    else
        resultPrinter << stepCounter << "," << Type[int(word.type)] << "," << word.value << endl;
    /*if (word.type == LEXICAL_TYPE::UNKNOWN)
        resultPrinter << "    ****** warning! ******";
    resultPrinter << endl;*/
    stepCounter++;
}


