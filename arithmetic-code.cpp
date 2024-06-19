
#include <iostream>
#include <fstream>
#include <cassert>
#include <climits>
#include <gmp.h>
#include <arpa/inet.h>

using namespace std;

#ifndef COMMIT_ID
#define COMMIT_ID "UNKNOWN"
#endif

#define ERR(...) { \
    cerr << "ERROR: "; \
    cerr << "commit `" << COMMIT_ID << "`, "; \
    cerr << "file `" << __FILE__ << "`, "; \
    cerr << "line " << __LINE__ << ": "; \
    cerr << __VA_ARGS__; \
    cerr << endl; \
    exit(1); \
}

#define ASSERT(condition) { \
    if(!(condition)){ \
        ERR("Assertion failed"); \
    } \
}

// used when the end user did something wrong
#define USER_ERR(...) { \
    cerr << "ERROR: "; \
    cerr << __VA_ARGS__; \
    cerr << endl; \
    exit(1); \
}

#define SYMBOL_COUNTS_TYPE array<uint32_t, 256>
// we need 1 count for each byte -> 2**8 -> length needs to be 256
// with uint32_t we can count 2**32 -> 4294967296 times each character, so if a file consist all of the same character, the can process at max a 4GiB file

#define SYMBOL_BOTS_TYPE array<uint32_t, 256>

SYMBOL_BOTS_TYPE calculate_symbol_bots(const SYMBOL_COUNTS_TYPE & symbol_counts){

    SYMBOL_BOTS_TYPE symbol_bots = {};

    {
        uint32_t base = 0;

        for(size_t symbol_idx = 0; symbol_idx < symbol_counts.size(); ++symbol_idx){
            uint32_t symbol_count = symbol_counts.at(symbol_idx);

            uint32_t bot = base;

            base = base + symbol_count;

            symbol_bots.at(symbol_idx) = bot;
        }
    }

    return symbol_bots;

}

uint32_t calculate_total_number_of_symbols(const SYMBOL_COUNTS_TYPE & symbol_counts){

    uint32_t total_number_of_symbols = 0;

    {
        for(uint32_t count : symbol_counts){
            total_number_of_symbols += count;
            ASSERT(total_number_of_symbols >= count); // overflow
        }
    }

    return total_number_of_symbols;

}

void calculate_possible_combinations(mpz_t & combinations, const SYMBOL_COUNTS_TYPE & symbol_counts, uint32_t total_number_of_symbols){

    // formula for calculating all possible combinations is:
    // n! / (a! * b! * c! * ...)
    // where n is the total number of symbols (including duplicates)
    // a, b, c, ... are each of the symbol counts

    // calc dividend

    mpz_t dividend;
    mpz_init(dividend);
    mpz_fac_ui(dividend, total_number_of_symbols);

    // calc divisor

    mpz_t divisor;
    mpz_init(divisor);
    // mpz_set_str(divisor, "1", 10);
    mpz_set_ui(divisor, 1u);

    for(uint32_t count : symbol_counts){
        mpz_t count_factorial;
        mpz_init(count_factorial);

        mpz_fac_ui(count_factorial, count);

        mpz_mul(divisor, divisor, count_factorial);

        mpz_clear(count_factorial);
    }

    // calc total combinations

    mpz_tdiv_q(combinations, dividend, divisor);

    // cout << "combinations: ";
    // mpz_out_str(stdout, 10, combinations);
    // cout << endl;

    mpz_clear(dividend);
    mpz_clear(divisor);

}

void update_probabilities_based_on_used_up_symbol(unsigned char symbol, uint32_t & number_of_remaining_symbols, SYMBOL_COUNTS_TYPE & symbol_counts, SYMBOL_BOTS_TYPE & symbol_bots){

    ASSERT(number_of_remaining_symbols > 0); // unreachable
    number_of_remaining_symbols -= 1;

    ASSERT(symbol_counts.at(symbol) > 0); // unreachable
    symbol_counts.at(symbol) -= 1;

    unsigned char sym = 0;
    
    while(true){

        if(sym > symbol){

            if(symbol_bots[sym] != 0){
                symbol_bots[sym] -= 1;
            }

        }

        sym += 1;
        if(sym <= 0){
            break;
        }

    }

}

void encode(const string & file_to_compress, const string & file_compressed){

    cout << "counting symbols..." << endl;

    SYMBOL_COUNTS_TYPE symbol_counts = {};

    {

        ifstream file_in;
        file_in.open(file_to_compress, ios::binary);
        if(!file_in.is_open()){
            USER_ERR("Could not open file `" << file_to_compress << "`");
        }

        while(true){

            unsigned char symbol;
            file_in.read(reinterpret_cast<char*>(&symbol), 1);

            if(file_in.gcount() <= 0){
                break;
            }

            symbol_counts.at(symbol) += 1;
            ASSERT(symbol_counts.at(symbol) > 0);
        }

    }

    cout << "writing symbol counts..." << endl;

    {
        ofstream file_out;
        file_out.open(file_compressed, ios::binary);
        ASSERT(file_out.is_open());

        for(uint32_t count : symbol_counts){
            uint32_t big_endian = htonl(count);
            file_out.write(reinterpret_cast<char *>(&big_endian), sizeof(big_endian));
        }
    }

    cout << "calculating ranges..." << endl;

    SYMBOL_BOTS_TYPE symbol_bots = calculate_symbol_bots(symbol_counts);

    cout << "calculating number of symbols..." << endl;

    uint32_t total_number_of_symbols = calculate_total_number_of_symbols(symbol_counts);

    cout << "calculating possible combinations..." << endl;

    mpz_t combinations;
    mpz_init(combinations);
    calculate_possible_combinations(combinations, symbol_counts, total_number_of_symbols);

    cout << "encoding..." << endl;

    mpz_t bot;
    mpz_init(bot);
    mpz_set_ui(bot, 0u);

    {

        uint32_t number_of_remaining_symbols = total_number_of_symbols;

        ifstream file_in;
        file_in.open(file_to_compress, ios::binary);
        ASSERT(file_in.is_open());

        mpz_t top;
        mpz_init_set(top, combinations);

        mpz_clear(combinations);

        while(true){

            unsigned char symbol;
            file_in.read(reinterpret_cast<char*>(&symbol), 1);

            if(file_in.gcount() <= 0){
                break;
            }

            // remaining_combinations = top - bot
            mpz_t remaining_combinations;
            mpz_init_set(remaining_combinations, top);
            mpz_sub(remaining_combinations, remaining_combinations, bot);

            uint32_t symbol_count = symbol_counts.at(symbol);
            uint32_t symbol_bot = symbol_bots.at(symbol);

            // symbol_bot_scaled = remaining_combinations * symbol_bot / number_of_remaining_symbols
            mpz_t symbol_bot_scaled;
            mpz_init_set(symbol_bot_scaled, remaining_combinations);
            mpz_mul_ui(symbol_bot_scaled, symbol_bot_scaled, symbol_bot);
            mpz_div_ui(symbol_bot_scaled, symbol_bot_scaled, number_of_remaining_symbols); // TODO this shouldn't leave a remainder, be we can add an assert to be sure

            // symbol_count_scaled = remaining_combinations * symbol_count / number_of_remaining_symbols
            mpz_t symbol_count_scaled;
            mpz_init_set(symbol_count_scaled, remaining_combinations);
            mpz_mul_ui(symbol_count_scaled, symbol_count_scaled, symbol_count);
            mpz_div_ui(symbol_count_scaled, symbol_count_scaled, number_of_remaining_symbols); // TODO this shouldn't leave a remainder, be we can add an assert to be sure

            mpz_add(bot, bot, symbol_bot_scaled);
            mpz_add(top, bot, symbol_count_scaled);

            mpz_clear(remaining_combinations);
            mpz_clear(symbol_bot_scaled);
            mpz_clear(symbol_count_scaled);

            update_probabilities_based_on_used_up_symbol(symbol, number_of_remaining_symbols, symbol_counts, symbol_bots);

        }

        // TODO? test and see if we are to add all scaled symbols, are we going to get `combinations`

    }

    cout << "writing encoded data..." << endl;

    {
        FILE* file_out = fopen(file_compressed.c_str(), "ab");
        ASSERT(file_out);

        mpz_out_raw(file_out, bot);

        fclose(file_out);
    }

}

void decode(const string & file_compressed, const string & file_regenerated){

    cout << "opening compressed file..." << endl;

    FILE * file_in = fopen(file_compressed.c_str(), "rb");
    ASSERT(file_in);

    cout << "reading symbol counts..." << endl;

    SYMBOL_COUNTS_TYPE symbol_counts = {};

    {
        for(uint32_t & count : symbol_counts){

            uint32_t big_endian;
            ASSERT( fread(&big_endian, sizeof(big_endian), 1, file_in) == 1 );

            count = ntohl(big_endian);
        }
    }

    cout << "calculating symbol ranges..." << endl;

    SYMBOL_BOTS_TYPE symbol_bots = calculate_symbol_bots(symbol_counts);

    cout << "calculating number of symbols..." << endl;

    uint32_t total_number_of_symbols = calculate_total_number_of_symbols(symbol_counts);

    cout << "calculating possible combinations..." << endl;

    mpz_t combinations;
    mpz_init(combinations);
    calculate_possible_combinations(combinations, symbol_counts, total_number_of_symbols);

    cout << "reading encoded data..." << endl;

    mpz_t num;
    mpz_init(num);
    mpz_inp_raw(num, file_in);

    cout << "closing compressed file..." << endl;

    fclose(file_in);

    cout << "decoding..." << endl;

    {

        uint32_t number_of_remaining_symbols = total_number_of_symbols;

        ofstream file_out;
        file_out.open(file_regenerated, ios::binary);
        ASSERT(file_out.is_open());

        mpz_t bot;
        mpz_init_set_ui(bot, 0u);

        mpz_t top;
        mpz_init_set(top, combinations);

        for(uint32_t i = 0; i < total_number_of_symbols; ++i){ // tuk ima 1 edge case v koito toq look da stane infinite
            
            // remaining_combinations = top - bot
            mpz_t remaining_combinations;
            mpz_init_set(remaining_combinations, top);
            mpz_sub(remaining_combinations, remaining_combinations, bot);

            bool symbol_found = false;

            for(size_t symbol_s = 0; symbol_s < symbol_counts.size(); ++symbol_s){

                uint32_t symbol_count = symbol_counts.at(symbol_s);

                // using a vector with the remaining unique symbols, rather than having this if here actually makes the process (a tiny bit) slower
                if(symbol_count <= 0){
                    continue;
                }

                uint32_t symbol_bot = symbol_bots.at(symbol_s);

                // symbol_bot_scaled = remaining_combinations * symbol_bot / number_of_remaining_symbols
                mpz_t symbol_bot_scaled;
                mpz_init_set(symbol_bot_scaled, remaining_combinations);
                mpz_mul_ui(symbol_bot_scaled, symbol_bot_scaled, symbol_bot);
                mpz_div_ui(symbol_bot_scaled, symbol_bot_scaled, number_of_remaining_symbols); // TODO this shouldn't leave a remainder, be we can add an assert to be sure

                // symbol_count_scaled = remaining_combinations * symbol_count / number_of_remaining_symbols
                mpz_t symbol_count_scaled;
                mpz_init_set(symbol_count_scaled, remaining_combinations);
                mpz_mul_ui(symbol_count_scaled, symbol_count_scaled, symbol_count);
                mpz_div_ui(symbol_count_scaled, symbol_count_scaled, number_of_remaining_symbols); // TODO this shouldn't leave a remainder, be we can add an assert to be sure

                mpz_t new_bot;
                mpz_init(new_bot);
                mpz_t new_top;
                mpz_init(new_top);

                mpz_add(new_bot, bot, symbol_bot_scaled);
                mpz_add(new_top, new_bot, symbol_count_scaled);

                if(mpz_cmp(num, new_bot) >= 0){ // num >= new_bot
                    if(mpz_cmp(num, new_top) < 0){ // num < new_top
                        symbol_found = true;
                    }
                }

                if(symbol_found){
                    unsigned char symbol_uc = static_cast<unsigned char>(symbol_s);
                    file_out.write(reinterpret_cast<char *>(&symbol_uc), 1);

                    mpz_set(bot, new_bot);
                    mpz_set(top, new_top);

                    update_probabilities_based_on_used_up_symbol(symbol_uc, number_of_remaining_symbols, symbol_counts, symbol_bots);
                }

                mpz_clear(new_bot);
                mpz_clear(new_top);

                mpz_clear(symbol_bot_scaled);
                mpz_clear(symbol_count_scaled);

                if(symbol_found){
                    break;
                }

            }

            if(!symbol_found){
                file_out.close();
                ASSERT(false);
            }

            mpz_clear(remaining_combinations);

        }

    }

    mpz_clear(combinations);

}

#define ACTION_ENCODE "enc"
#define ACTION_DECODE "dec"

int main(int argc, char * * argv){

    if(argc != 4){
        USER_ERR(
            endl <<
            "    You need to provice exactly 3 arguments:" << endl <<
            "        action (`" << ACTION_ENCODE << "` to encode/compress, or `" << ACTION_DECODE "` to decode/decompress)" << endl <<
            "        input file" << endl <<
            "        output file" << endl
        );
    }

    string action = argv[1];
    string file_in = argv[2];
    string file_out = argv[3];

    if(action == ACTION_ENCODE){

        encode(file_in, file_out);

    }else if(action == ACTION_DECODE){

        decode(file_in, file_out);

    }else{

        USER_ERR(
            "Invalid action `" << action << "`" << endl <<
            "Only valid actions are `" << ACTION_ENCODE << "` and `" << ACTION_DECODE << "`"
        );

    }

    return 0;
}
