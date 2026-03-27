#include "../includes/essentials.h"
#include "../includes/debug_utils.h"
#include "../includes/search_utils_unary.h"
#include "../includes/search_utils_binary.h"
#include <functional>

static function<wire(wire, wire)> gate;

wire const_neg(wire x){return T_NEG;}
wire const_zero(wire x){return T_ZERO;}
wire const_pos(wire x){return T_POS;}

wire shf_1(wire x){
    vector<wire> vals = {T_ZERO, T_NEG, T_POS};
    return vals[x];
} 

wire rel_1(wire x){
    vector<wire> vals = {T_POS, T_ZERO, T_NEG};
    return vals[x];
}


wire p1(wire x){
    return const_pos(gate(T_NEG, x));
}

wire p2(wire x){
    return rel_1(gate(T_ZERO, shf_1(x)));
}

wire p3(wire x){
    return gate(x, T_POS);
}

//   + + +
//   + 0 0
//   + 0 -

int main() {
    string g_string = "0++0-0-+-";
    gate = string_to_gate(g_string);
    print_matrix(gate);

    print_vector(get_vector(p1));cout << endl;
    print_vector(get_vector(p2));cout << endl;
    print_vector(get_vector(p3));cout << endl;
    return 0;
}
