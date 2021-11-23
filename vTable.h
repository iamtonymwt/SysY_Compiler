#ifndef MIPS_VTABLE_H
#define MIPS_VTABLE_H
#include <string>
#include <vector>
#include <map>
using namespace std;


struct Variable{
    int num;
    string rName = "";
    string fName = "";
    int dimCount; //0 1 2
    int dim[2];
    int level;
    bool valid;
    int address;
    string func = "";
    vector<string> value;  //存const的初值 var不存
};

struct Function{
    string ident;
    int type;
    int length;
    vector<Variable> params;
};

string addNewV();
string addOldV(string rName, int dim, int *d = nullptr);
string getOldVName(string rName);
void checkvTable();
int countOffset(int dim, int *d);


#endif //MIPS_VTABLE_H
