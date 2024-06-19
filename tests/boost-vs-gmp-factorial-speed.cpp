
#include <iostream>
#include <fstream>
#include <cassert>
#include <climits>
#include <gmp.h>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/math/special_functions/factorials.hpp>

using namespace std;
using namespace boost::multiprecision;

#define ERR(...) { \
    cerr << "ERROR: "; \
    cerr << "file `" << __FILE__ << "` "; \
    cerr << "line " << __LINE__ << ": "; \
    cerr << __VA_ARGS__; \
    cerr << endl; \
    exit(1); \
}

#define UNREACHABLE() { \
    ERR("Unreachable code reached") \
}

#define ASSERT(condition) { \
    if(!(condition)){ \
        ERR("Assertion failed"); \
    } \
}

int main(){

    // boost
    // time taken: 0m2,625s

    cpp_int a("1");

    for(int i=2; i<=80'963; ++i){
        a *= i;
    }

    cout << "result: " << a << endl;



    // // gmp
    // // time taken: 0m0,039s

    // mpz_t a;
    // mpz_init(a);

    // unsigned long int n = 80'963;

    // mpz_fac_ui(a, n);

    // cout << "result: ";
    // mpz_out_str(stdout, 10, a);
    // cout << endl;

    // mpz_clear(a);

    return 0;
}
