#include "../includes/essentials.h"
#include "../includes/debug_utils.h"

wire Arb(wire a, wire b){
    static const wire arbMatrix[3][3] = {
        {T_POS,  T_POS,  T_POS},
        {T_POS,  T_NEG, T_ZERO},
        {T_POS,  T_ZERO, T_NEG}
    };
    return arbMatrix[a][b];
}

wire Un1(wire a){
    wire result = Arb(a,a);
    return result;
}

wire Un11(wire a){
    wire result = Arb(a, T_NEG);
    return result;
}

wire Un12(wire a){
    wire result = Arb(a, T_ZERO);
    return result;
}

wire Un13(wire a){
    wire result = Arb(a, T_POS);
    return result;
}

wire Un21(wire a){
    wire result = Arb(T_NEG, a);
    return result;
}

wire Un22(wire a){
    wire result = Arb(T_ZERO, a);
    return result;
}

wire Un23(wire a){
    wire result = Arb(T_POS, a);
    return result;
}

wire AnyUn(wire a){
    wire notA = Not(a);
    
    wire isA_pos = isPos(a);
    wire isA_neg = isNeg(a);
    wire isA_zero = isZero(a);

    wire and_n = And(isA_neg, T_ZERO);
    wire and_z = And(isA_zero, T_POS);
    wire and_p = And(isA_pos, T_ZERO);
    
    wire result = T_NEG;
    result = Or(result, and_n);
    result = Or(result, and_z);
    result = Or(result, and_p);

    return result;
}

wire test_mid(wire x){
    return Arb(Arb(T_ZERO,x), Arb(T_ZERO,x));
}

int main(){
    print_matrix(Arb);
    printf("\n");
    print_vector(get_vector(test_mid));printf("\n");
    return 0;
}
