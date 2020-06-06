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
        �ر��ļ���
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
    if (ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z' || '_' == ch) //�����»���
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
        lineCounter++;//һ�ж�ȡ��� ������1
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
        nextBuffer = 2;//��һ��ʹ��buffer2
    }
    else {
        buffer2.clear();
        buffer1 = currentBuffer;
        currentBuffer = buffer2;
        nextBuffer = 1;//��һ��ʹ��buffer1
    }
    while (sum < 120 && !codeReader.eof()) {//����һ��buffer
        ch = getNextChar();
        if (ch == '/') {//����ע��

            commentMode = true;
            nextChar = codeReader.peek();//������ָ�벻����ƶ�ֻ�ǹ۲� get����ƶ� 
            if (nextChar == '/') {//˫б��ע��

                while (getNextChar() != '\n' && !codeReader.eof());//���س�Ϊֹ
                commentMode = false;
            }
            else if (nextChar == '*') {// /**/ע��

                while (!codeReader.eof()) {

                    while (getNextChar() != '*' && !codeReader.eof());//��*Ϊֹ
                    //UNKNOWN
                    if (codeReader.eof()) {//����������ע��

                        cout << "ERROR End" << endl;
                    }

                    //  /**/ ע�ͽ���
                    if (getNextChar() == '/') {
                        commentMode = false;
                        break;
                    }
                }
            }
            else {//���Ǹ�����

                currentBuffer.push_back(ch);
                sum++;
            }
        }
        else {//��ע��
            currentBuffer.push_back(ch);
            sum++;
            if (ch == ' ') {//ȥ�ո�
                //����һ�����ǿո�Ϊֹ
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
    if (bufferPoint >= bufferSum && fileEnd == true) {//����EOF�˾ͷ�����һ�����������ڵ�
        return currentBuffer[bufferPoint - 1];
    }
    unsigned char ch;
    if (bufferPoint == 120) {
        if (backBuffer == true) {//�����Ǵ���һ���������˻�����,���ʱ��һ���������Ѿ�װ�����
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
    else {//����������Ҫ�˵���һ��������
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
                if (isDigit(nextChar)) //��һ��������
                {
                    str_token += nextChar;
                }
                else //��һ����������
                {
                    if (isDot(nextChar)) //��С��������ѭ���жϺ��������
                    {
                        str_token += nextChar;
                        flag = 1; //��С��
                        continue;
                    }
                    else //����С��,��int����
                    {
                        backBufferPoint();//�����ļ�ָ�����,��Ϊ�����nextChar
                        word.value = str_token;
                        if (flag == 0)
                            word.type = LEXICAL_TYPE::CINT; //int��
                        else if (flag == 1)
                            word.type = LEXICAL_TYPE::CFLOAT; //float��
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
                if ((!isDigit(nextChar)) && (!isLetter(nextChar))) //��һ��������ĸ�����֣���Ҫ�˻�
                {
                    backBufferPoint();
                    //�ж��Ƿ�Ϊ�ؼ��֡���������
                    if (KEYWORDS.find(str_token) != KEYWORDS.end()) //�ڹؼ�����
                        word.type = LEXICAL_TYPE::KEYWORD;
                    else if (VAR_TYPES.find(str_token) != VAR_TYPES.end()) //�ڱ���������
                        word.type = LEXICAL_TYPE::VARTYPE;
                    else //��ʶ��
                        word.type = LEXICAL_TYPE::IDENTIFIER;

                    word.value = str_token;
                    return word;
                }
                else  //��һ��������ĸ�����֣�����ѭ��
                    str_token += nextChar;
            }
        }
        else if (BORDERS.find(ch) != BORDERS.end()) {
            word.type = LEXICAL_TYPE::BORDER; //�߽�
            word.value = ch;
            return word;
        }
        else if (isSingleCharOperator(ch))//�����������
        {
            word.type = LEXICAL_TYPE::OPERATOR; //������
            word.value = ch;
            return word;
        }
        else if (isDoubleCharOperatorPre(ch))//> = < !�ж��Ƿ�Ϊ˫���������
        {
            str_token += ch;
            unsigned char nextChar = getBufferChar();

            if ('=' == nextChar) //ȷ����˫��������� == >= <= !=
            {
                str_token += nextChar;
                word.type = LEXICAL_TYPE::OPERATOR; //������
                word.value = str_token;
                return word;
            }
            else
            {
                backBufferPoint();//�����ļ�ָ�����,��Ϊ�����nextChar
                if (ch != '!')//�Ǹ������Ų����� ����!= �Ǳض�Ϊ>/</ =
                {
                    word.type = LEXICAL_TYPE::OPERATOR; //������
                    word.value = str_token;
                    return word;
                }
                else// ! δ����ķ���
                {
                    word.type = LEXICAL_TYPE::UNKNOWN; //δ֪������
                    word.value = str_token;
                    return word;
                }
            }
        }
        else if (isBlank(ch))
        {
            continue;
        }
        else if (ch == '\'') //��ͷ���ַ��ͱ�־\'
        {
            str_token += ch;
            while (!(bufferPoint >= bufferSum && fileEnd == true))
            {
                unsigned char nextChar = getBufferChar();
                if (isDigit(nextChar) || isLetter(nextChar) || isBlank(nextChar)) //���֣���ĸ��' ',\n,\t,255
                    str_token += nextChar;
                else if (nextChar == '\'') {//\'(�ַ�������־)
                    str_token += nextChar;
                    word.value = str_token;
                    word.type = LEXICAL_TYPE::CCHAR; //char�ͳ���
                    return word;
                }
            }
        }
        else
        {
            word.type = LEXICAL_TYPE::UNKNOWN;//δ֪
            word.value = ch;
            /*cout << ch << "***********************" << endl;*/
            //word.value += "," + std::to_string(lineCounter);//���� �м���
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
    //�˴���wordStr��Ҫ���﷨�������ķ����ս����ͬ
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
        resultPrinter << stepCounter << "," << Type[int(word.type)] << "," << "��" << endl;
    else
        resultPrinter << stepCounter << "," << Type[int(word.type)] << "," << word.value << endl;
    /*if (word.type == LEXICAL_TYPE::UNKNOWN)
        resultPrinter << "    ****** warning! ******";
    resultPrinter << endl;*/
    stepCounter++;
}


