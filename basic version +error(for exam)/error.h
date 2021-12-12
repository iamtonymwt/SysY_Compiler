#ifndef ERROR_CPP_ERROR_H
#define ERROR_CPP_ERROR_H

void lookErrorA();
void lookErrorBv();
void lookErrorBf();
void lookErrorCv();
void lookErrorCf();
void lookErrorD(string funcIdent, int rCount, int funcNameLine);
void lookErrorE(string funcIdent, int kind, int rCount, int funcNameLine);
void lookErrorF(int returntkLine);
void lookErrorG(bool isFuncFollow, bool isLatestReturn);
void lookErrorH(bool isAssign, string ident);
bool lookErrorI(int symbol);
bool lookErrorJ(int symbol);
bool lookErrorK(int symbol);
void lookErrorL(string formatString, int expCount, int printftkLine);
void lookErrorM();

#endif //ERROR_CPP_ERROR_H
