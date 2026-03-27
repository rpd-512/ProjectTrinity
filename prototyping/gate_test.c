#include "../includes/essentials.h"
#include "../includes/debug_utils.h"

wire Con_Decoder(wire a, wire b)
{
    wire notA = Not(a);
    wire notB = Not(b);

    wire isA_pos = isPos(a);
    wire isA_neg = isNeg(a);
    wire isA_zero = isZero(a);

    wire isB_pos = isPos(b);
    wire isB_neg = isNeg(b);
    wire isB_zero = isZero(b);

    wire and_nn = And(isA_neg, isB_neg);
    wire and_nz = And(isA_neg, isB_zero);
    wire and_np = And(isA_neg, isB_pos);
    wire and_zn = And(isA_zero, isB_neg);
    wire and_zz = And(isA_zero, isB_zero);
    wire and_zp = And(isA_zero, isB_pos);
    wire and_pn = And(isA_pos, isB_neg);
    wire and_pz = And(isA_pos, isB_zero);
    wire and_pp = And(isA_pos, isB_pos);

    and_nn = And(and_nn, T_NEG);
    and_nz = And(and_nz, T_ZERO);
    and_np = And(and_np, T_ZERO);
    and_zn = And(and_zn, T_ZERO);
    and_zz = And(and_zz, T_ZERO);
    and_zp = And(and_zp, T_ZERO);
    and_pn = And(and_pn, T_ZERO);
    and_pz = And(and_pz, T_ZERO);
    and_pp = And(and_pp, T_POS);

    wire result = T_NEG;
    result = Or(result, and_nn);
    result = Or(result, and_nz);
    result = Or(result, and_np);
    result = Or(result, and_zn);
    result = Or(result, and_zz);
    result = Or(result, and_zp);
    result = Or(result, and_pn);
    result = Or(result, and_pz);
    result = Or(result, and_pp);

    return result;
}


wire Con(wire a, wire b){
    wire xnor_res = Xnor(a, b);
    wire zero_or = Or(xnor_res, T_ZERO);

    wire or_ab = Or(a, b);

    wire result = Xnor(zero_or, or_ab);
    return result;
}

wire Any(wire a, wire b){
    static const wire anyMatrix[3][3] = {
        {T_NEG, T_NEG, T_ZERO},
        {T_NEG, T_ZERO, T_POS},
        {T_ZERO, T_POS, T_POS}
    };
    return anyMatrix[a][b];
}


wire Sum(wire a, wire b){
    wire xnor_res = Xor(a, b);
    wire zero_or = And(xnor_res, T_ZERO);

    return zero_or;
}

int main(){
    print_matrix(Con);
    printf("\n");
    print_matrix(Sum);
    return 0;
}
