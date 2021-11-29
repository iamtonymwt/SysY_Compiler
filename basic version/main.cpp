#include <iostream>
#include <fstream>
#include "lexical.h"
#include "grammer.h"
#include "midcode.h"
#include "mipscode.h"
using namespace std;

extern string fileContent;
ifstream inputFile;
//ofstream outputFile;
ofstream midCodefile;
ofstream mipsCodefile;

int main() {

    inputFile.open("testfile.txt", ios::in);
//    outputFile.open("output.txt", ios::out | ios::ate);
    midCodefile.open("midCode.txt", ios::out | ios::ate);
    mipsCodefile.open("mips.txt", ios::out | ios::ate);

    string tempIn;
    while(getline(inputFile, tempIn)) {
        fileContent.append(tempIn);
        fileContent.append("\n");
    }

    neoLexicalAnalysis();

    int re = getsymNP();
    if (re == 1) {
        Gprocess();    //递归下降，生成midcode
    }

    outputMidCode();  //midcode output
    genMipsCode();      //midcode -> mipscode
    outputMipsCode();   //mipscode output

    inputFile.close();
//    outputFile.close();
//    midCodefile.close();
    mipsCodefile.close();

    return 0;
}
