#pragma once
#ifndef LEXICAL_LEXICAL_H
#define LEXICAL_LEXICAL_H

struct CompSymbol{
    int symbol = 0;
    std::string stringContent;
};

bool isBlank();
bool isNewLine();
bool isLetter();
bool isDigit();
bool isPlus();
bool isMinu();
bool isMult();
bool isDiv();
bool isMod();
bool isAnd();
bool isOr();
bool isLss();
bool isGre();
bool isExcla();
bool isAssign();
bool isSemicn();
bool isComma();
bool isLparent();
bool isRparent();
bool isLbrack();
bool isRbrack();
bool isLbrace();
bool isRbrace();
bool isDquo();
bool isEOF();
//basic narrating functions
void clearToken();
void catchToken();
void getCh();
void retract();
int isResWord();
//main lexical analysis
int tokenAnalysis();
int output2file();
int neoLexicalAnalysis();
int getsymP();
int getsymNP();
int print();

#endif //LEXICAL_LEXICAL_H
