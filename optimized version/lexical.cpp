#include<iostream>
#include<cstring>
#include <string>
#include "lexical.h"
#include <fstream>
#include <vector>

using namespace std;

enum resType {
    MAINTK, CONSTTK, INTTK, BREAKTK, CONTINUETK, IFTK, ELSETK, WHILETK, GETINTTK,
    PRINTFTK, RETURNTK,VOIDTK, // match with reword order
    IDENFR, INTCON, STRCON, NOT, AND, OR, PLUS, MINU, MULT, DIV, MOD, LSS, LEQ,
    GRE, GEQ, EQL, NEQ, ASSIGN, SEMICN, COMMA, LPARENT, RPARENT, LBRACK, RBRACK, LBRACE, RBRACE,
    ANNOTATION
};
char resWord[20][10] {
        "main", "const", "int", "break", "continue", "if", "else", "while", "getint",
        "printf", "return", "void" // match with resType order
};
int resWordCount = 12;

char ch;
char token[1000000000];
int tokenI = 0;

vector<CompSymbol> symbolList;
int symbolListI = 0;
int symbolListC = 0;


int symbol;
string neotoken;
int indexs = 0;
int line = 1;
string fileContent;
//extern ofstream outputFile;

//basic item recognition
bool isBlank() {
    return (ch == ' ' || ch == '\n' || ch == '\t' || ch == '\r' || ch == '\f' || ch == '\v');
}
bool isNewLine() {
    return (ch == '\n');
}
bool isLetter(){
    return ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch == '_'));
}
bool isDigit(){
    return (ch >= '0' && ch <= '9');
}
bool isPlus(){
    return (ch == '+');
}
bool isMinu(){
    return (ch == '-');
}
bool isMult(){
    return (ch == '*');
}
bool isDiv(){
    return (ch == '/');
}
bool isMod(){
    return (ch == '%');
}
bool isAnd(){
    return (ch == '&');
}
bool isOr(){
    return (ch == '|');
}
bool isLss(){
    return (ch == '<');
}
bool isGre(){
    return (ch == '>');
}
bool isExcla(){
    return (ch == '!');
}
bool isAssign(){
    return (ch == '=');
}
bool isSemicn(){
    return (ch == ';');
}
bool isComma(){
    return (ch == ',');
}
bool isLparent(){
    return (ch == '(');
}
bool isRparent(){
    return (ch == ')');
}
bool isLbrack(){
    return (ch == '[');
}
bool isRbrack(){
    return (ch == ']');
}
bool isLbrace(){
    return (ch == '{');
}
bool isRbrace(){
    return (ch == '}');
}
bool isDquo(){
    return (ch == '\"');
}
bool isEOF(){
    return (indexs >= fileContent.size());
}

//basic narrating functions
void clearToken(){
    tokenI = 0;
}
void catchToken(){
    token[tokenI++] = ch;
}
void getCh(){
    ch = fileContent[indexs++];
    if (isNewLine()) {
        line++;
    }
}
void retract(){
    if (isNewLine()) {
        line--;
    }
    indexs--;
}
int isResWord(){
    for (int i = 0; i < resWordCount; i++) {
        if (strcmp(resWord[i], token) == 0) {
            return i;
        }
    }
    return -1;
}

//main lexical analysis
int tokenAnalysis() {
    clearToken();
    getCh();
    while (isBlank()) {
        getCh();
    }
    if (isEOF()) {
        return -1;
    }
    if (isLetter()) {
        while (isLetter() || isDigit()) {
            catchToken();
            getCh();
        }
        retract();
        token[tokenI] = '\0';
        int resValue = isResWord();
        if (resValue == -1) {
            symbol = IDENFR;
        }
        else {
            symbol = (resType)resValue;
        }
        return 1;
    }
    else if (isDigit()) {
        while (isDigit()) {
            catchToken();
            getCh();
        }
        retract();
        token[tokenI] = '\0';
        symbol = INTCON;
        return 1;
    }
    else if (isDquo()) {
        getCh();
        while (!isDquo()) {
            catchToken();
            getCh();
        }
        symbol = STRCON;
        token[tokenI] = '\0';
        return 1;
    }
    else if (isAnd()) {
        getCh();
        if (isAnd()) {
            symbol = AND;
            return 1;
        }
        else {
            return -1;
        }
    }
    else if (isOr()) {
        getCh();
        if (isOr()) {
            symbol = OR;
            return 1;
        }
        else {
            return -1;
        }
    }
    else if (isPlus()) {
        symbol = PLUS;
        return 1;
    }
    else if (isMinu()) {
        symbol = MINU;
        return 1;
    }
    else if (isMult()) {
        symbol = MULT;
        return 1;
    }
    else if (isDiv()) {
        getCh();
        if (isDiv()) {  // //
            while (!isNewLine()) {
                getCh();
            }
            symbol = ANNOTATION;
            return 1;
        }
        else if (isMult()) {    // /*
            while(true) {
                getCh();
                if (isMult()) {
                    getCh();
                    if (isDiv()){
                        symbol = ANNOTATION;
                        break;
                    }
                    retract();
                }
            }
            return 1;
        }
        retract();
        symbol = DIV;
        return 1;
    }
    else if (isMod()) {
        symbol = MOD;
        return 1;
    }
    else if (isLss()) {
        getCh();
        if (isAssign()) {
            symbol = LEQ;
        }
        else {
            symbol = LSS;
            retract();
        }
        return 1;
    }
    else if (isGre()) {
        getCh();
        if (isAssign()) {
            symbol = GEQ;
        }
        else {
            symbol = GRE;
            retract();
        }
        return 1;
    }
    else if (isExcla()) {
        getCh();
        if (isAssign()) {
            symbol = NEQ;
        }
        else {
            retract();
            symbol = NOT;
        }
        return 1;
    }
    else if (isAssign()) {
        getCh();
        if (isAssign()) {
            symbol = EQL;
        }
        else {
            symbol = ASSIGN;
            retract();
        }
        return 1;
    }
    else if (isSemicn()) {
        symbol = SEMICN;
        return 1;
    }
    else if (isComma()) {
        symbol = COMMA;
        return 1;
    }
    else if (isLparent()) {
        symbol = LPARENT;
        return 1;
    }
    else if (isRparent()) {
        symbol = RPARENT;
        return 1;
    }
    else if (isLbrack()) {
        symbol = LBRACK;
        return 1;
    }
    else if (isRbrack()) {
        symbol = RBRACK;
        return 1;
    }
    else if (isLbrace()) {
        symbol = LBRACE;
        return 1;
    }
    else if (isRbrace()) {
        symbol = RBRACE;
        return 1;
    }
    else {
        return -1;
    }
}

//int output2file() {
//    switch (symbol) {
//        case IDENFR:
//            outputFile << "IDENFR " << neotoken << endl;
//            break;
//        case INTCON:
//            outputFile << "INTCON " << neotoken << endl;
//            break;
//        case STRCON:
//            outputFile << "STRCON " << "\"" << neotoken << "\"" << endl;
//            break;
//        case MAINTK:
//            outputFile << "MAINTK main" << endl;
//            break;
//        case CONSTTK:
//            outputFile << "CONSTTK const" << endl;
//            break;
//        case INTTK:
//            outputFile << "INTTK int" << endl;
//            break;
//        case BREAKTK:
//            outputFile << "BREAKTK break" << endl;
//            break;
//        case CONTINUETK:
//            outputFile << "CONTINUETK continue" << endl;
//            break;
//        case IFTK:
//            outputFile << "IFTK if" << endl;
//            break;
//        case ELSETK:
//            outputFile << "ELSETK else" << endl;
//            break;
//        case NOT:
//            outputFile << "NOT !" << endl;
//            break;
//        case AND:
//            outputFile << "AND &&" << endl;
//            break;
//        case OR:
//            outputFile << "OR ||" << endl;
//            break;
//        case WHILETK:
//            outputFile << "WHILETK while" << endl;
//            break;
//        case GETINTTK:
//            outputFile << "GETINTTK getint" << endl;
//            break;
//        case PRINTFTK:
//            outputFile << "PRINTFTK printf" << endl;
//            break;
//        case RETURNTK:
//            outputFile << "RETURNTK return" << endl;
//            break;
//        case PLUS:
//            outputFile << "PLUS +" << endl;
//            break;
//        case MINU:
//            outputFile << "MINU -" << endl;
//            break;
//        case VOIDTK:
//            outputFile << "VOIDTK void" << endl;
//            break;
//        case MULT:
//            outputFile << "MULT *" << endl;
//            break;
//        case DIV:
//            outputFile << "DIV /" << endl;
//            break;
//        case MOD:
//            outputFile << "MOD %" << endl;
//            break;
//        case LSS:
//            outputFile << "LSS <" << endl;
//            break;
//        case LEQ:
//            outputFile << "LEQ <=" << endl;
//            break;
//        case GRE:
//            outputFile << "GRE >" << endl;
//            break;
//        case GEQ:
//            outputFile << "GEQ >=" << endl;
//            break;
//        case EQL:
//            outputFile << "EQL ==" << endl;
//            break;
//        case NEQ:
//            outputFile << "NEQ !=" << endl;
//            break;
//        case ASSIGN:
//            outputFile << "ASSIGN =" << endl;
//            break;
//        case SEMICN:
//            outputFile << "SEMICN ;" << endl;
//            break;
//        case COMMA:
//            outputFile << "COMMA ," << endl;
//            break;
//        case LPARENT:
//            outputFile << "LPARENT (" << endl;
//            break;
//        case RPARENT:
//            outputFile << "RPARENT )" << endl;
//            break;
//        case LBRACK:
//            outputFile << "LBRACK [" << endl;
//            break;
//        case RBRACK:
//            outputFile << "RBRACK ]" << endl;
//            break;
//        case LBRACE:
//            outputFile << "LBRACE {" << endl;
//            break;
//        case RBRACE:
//            outputFile << "RBRACE }" << endl;
//            break;
//        default:
//            break;
//    }
//    return 0;
//}


int neoLexicalAnalysis() {
    while(tokenAnalysis() != -1) {
        if (symbol == ANNOTATION) {
            continue;
        }
        CompSymbol compSymbol;
        compSymbol.symbol = symbol;
        compSymbol.stringContent = token;
        symbolList.push_back(compSymbol);
        symbolListI ++;
    }
    symbolListC = symbolListI;
    symbolListI = 0;
    return 0;
}

int getsymP() {
    if (symbolListI == symbolListC) {
        return 0;
    }
    symbol = symbolList[symbolListI].symbol;
    neotoken = symbolList[symbolListI].stringContent;
//    output2file();
    symbolListI += 1;
    return 1;
}

int getsymNP() {
    if (symbolListI == symbolListC) {
        return 0;
    }
    symbol = symbolList[symbolListI].symbol;
    neotoken = symbolList[symbolListI].stringContent;
    symbolListI += 1;
    return 1;
}

//int print() {
//    output2file();
//    return 0;
//}