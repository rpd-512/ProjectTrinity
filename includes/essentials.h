#ifndef ESSENTIALS_H
#define ESSENTIALS_H

#include <stdint.h>
#include <stdio.h>
#include <limits.h>
#define T_NEG 0
#define T_ZERO 1
#define T_POS 2

typedef uint8_t wire;

wire Nand(wire a, wire b){
    static const wire nandMatrix[3][3] = {
        {T_POS, T_POS, T_POS},
        {T_POS, T_ZERO, T_ZERO},
        {T_POS, T_ZERO, T_NEG}
    };
    return nandMatrix[a][b];
}

wire Not(wire a){
    wire not_gate = Nand(a, a);
    return not_gate;
}

wire And(wire a, wire b){
    wire nand_res = Nand(a, b);
    wire result = Not(nand_res);
    return result;
}


wire Or(wire a, wire b){
    wire notA = Not(a);
    wire notB = Not(b);
    wire andNot = And(notA, notB);
    wire result = Not(andNot);
    return result;
}

wire Nor(wire a, wire b){
    wire or_res = Or(a, b);
    wire result = Not(or_res);
    return result;
}

wire Xor(wire a, wire b){
    wire notA = Not(a);
    wire notB = Not(b);
    wire and1 = And(a, notB);
    wire and2 = And(notA, b);
    wire orResult = Or(and1, and2);
    return orResult;
}

wire isPos(wire a){
    return a == T_POS ? T_POS : T_NEG;
}

wire isNeg(wire a){
    return a == T_NEG ? T_POS : T_NEG;
}

wire isZero(wire a){
    return a == T_ZERO ? T_POS : T_NEG;
}

wire Xnor(wire a, wire b){
    wire notA = Not(a);
    wire notB = Not(b);
    wire and1 = And(a, b);
    wire and2 = And(notA, notB);
    wire orResult = Or(and1, and2);
    return orResult;
}

#endif // ESSENTIALS_H