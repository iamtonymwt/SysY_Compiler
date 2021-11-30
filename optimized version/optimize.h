//
// Created by mawen on 2021/11/28.
//

#ifndef OPTIMIZE_OPTIMIZE_H
#define OPTIMIZE_OPTIMIZE_H

#endif //OPTIMIZE_OPTIMIZE_H

class Block{
public:
    int number;
    int start;
    int end;
    int nextBlock1;
    int nextBlock2;
    vector<midCode> midCodeVector;

    Block(int number, int start, int end) :
       number(number), start(start), end(end){
        nextBlock1 = -1;
        nextBlock2 = -1;
        midCodeVector = vector<midCode>();
    }

    void setMidCodeVector(vector<midCode> mid) {
        midCodeVector = mid;
    }
};

bool isInsVar(string ident);

void optimize();