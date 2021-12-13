#include <iostream>
#include <fstream>
#include "lexical.h"
#include "grammer.h"
#include "midcode.h"
#include "mipscode.h"
#include "optimize.h"
using namespace std;

extern string fileContent;
ifstream inputFile;
//ofstream outputFile;
ofstream midCodefile_origin;
ofstream midCodefile_optimize;
ofstream mipsCodefile_origin;
ofstream  mipsCodefile_optimize;
ofstream mipsCodefile;

int main() {

    inputFile.open("testfile.txt", ios::in);
//    outputFile.open("output.txt", ios::out | ios::ate);
    midCodefile_origin.open("midCode_origin.txt", ios::out | ios::ate);
    midCodefile_optimize.open("midCode_optimize.txt", ios::out | ios::ate);
    mipsCodefile_origin.open("mips_origin.txt", ios::out | ios::ate);
    mipsCodefile_optimize.open("mips_optimize.txt", ios::out | ios::ate);
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

    outputMidCode(midCodefile_origin);
    genMipsCode();
    outputMipsCode(mipsCodefile_origin);
    midcodeOptimize();
    outputMidCode(midCodefile_optimize);  //midcode output
    genMipsCode();      //midcode -> mipscode
    mipscodeOptimize();
    outputMipsCode(mipsCodefile_optimize);   //mipscode output
    outputMipsCode(mipsCodefile);



    inputFile.close();
//    outputFile.close();
    midCodefile_origin.close();
    midCodefile_optimize.close();
    mipsCodefile_origin.close();
    mipsCodefile_optimize.close();
    mipsCodefile.close();

    return 0;
}
