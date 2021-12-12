#include <iostream>
#include <fstream>
#include "lexical.h"
#include "mips_grammer.h"
#include "error_grammer.h"
#include "midcode.h"
#include "mipscode.h"
using namespace std;

extern bool hasError;

extern string fileContent;
ifstream inputFile;
ofstream errorFile;
ofstream midCodefile;
ofstream mipsCodefile;

int main() {

    inputFile.open("testfile.txt", ios::in);
    errorFile.open("error.txt", ios::out | ios::ate);
    midCodefile.open("midCode.txt", ios::out | ios::ate);
    mipsCodefile.open("mips.txt", ios::out | ios::ate);

    string tempIn;
    while(getline(inputFile, tempIn)) {
        fileContent.append(tempIn);
        fileContent.append("\n");
    }
    neoLexicalAnalysis();

    resetSymbolListI();
    int re = getsymNP();
    if (re == 1) {
        errorDetectProcess();    //错误处理
    }

    if (!hasError) {
        resetSymbolListI();
        re = getsymNP();
        if (re == 1) {
            Gprocess();    //递归下降，生成midcode
        }

        outputMidCode();  //midcode output
        genMipsCode();      //midcode -> mipscode
        outputMipsCode();   //mipscode output
    }

    inputFile.close();
    errorFile.close();
    midCodefile.close();
    mipsCodefile.close();

    return 0;
}
