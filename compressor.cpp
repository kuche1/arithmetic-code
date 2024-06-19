
#include <iostream>
#include <fstream>
#include <cassert>
#include <climits>
#include <gmp.h>

using namespace std;

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

#define PADDING 2000
// TODO
// there is some fucking bug somewhere, and I can't get my shit to work unless
// I put some padding bits

int main(int argc, char * * argv){

    if(argc != 4){
        cerr << "You need to provice exactly 2 arguments: the input file and the compressed output" << endl;
        exit(1);
    }

    string file_to_compress_str = argv[1]; // generate input file with: dd if=/dev/urandom of=./urandom bs=1k count=40
    string file_compressed_str = argv[2];
    string file_regenerated_str = argv[3];

    cout << "file_to_compress:" << file_to_compress_str << endl;
    cout << "file_compressed:" << file_compressed_str << endl;
    cout << "file_regenerated:" << file_regenerated_str << endl;

    // init counter

    array<uint32_t, 256> symbol_counts = {};
    // we need 1 count for each byte -> 2**8 -> length needs to be 256
    // with uint32_t we can count 2**32 -> 4294967296 times each character, so if a file consist all of the same character, the can process at max a 4GiB file

    cout << "counting symbols..." << endl;

    {

        ifstream file_in;
        file_in.open(file_to_compress_str, ios::binary);
        ASSERT(file_in.is_open());

        while(true){

            unsigned char symbol;
            file_in.read(reinterpret_cast<char*>(&symbol), 1);

            if(file_in.gcount() <= 0){
                break;
            }

            symbol_counts.at(symbol) += 1;
            ASSERT(symbol_counts.at(symbol) > 0);
        }

        for(int i=0; i<PADDING; ++i){
            symbol_counts.at(0) += 1;
            ASSERT(symbol_counts.at(0) > 0);
        }

    }

    cout << "calculating ranges..." << endl;

    array<uint32_t, 256> symbol_bots = {};

    {
        uint32_t base = 0;

        for(size_t symbol_idx = 0; symbol_idx < symbol_counts.size(); ++symbol_idx){
            uint32_t symbol_count = symbol_counts.at(symbol_idx);

            uint32_t bot = base;

            base = base + symbol_count;

            symbol_bots.at(symbol_idx) = bot;
        }
    }

    cout << "calculating combinations..." << endl;

    uint32_t total_number_of_symbols = 0;

    mpz_t combinations;
    mpz_init(combinations);

    {

        for(uint32_t count : symbol_counts){
            total_number_of_symbols += count;
            ASSERT(total_number_of_symbols >= count); // overflow
        }

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

    cout << "compressing..." << endl;

    {

        ifstream file_in;
        file_in.open(file_to_compress_str, ios::binary);
        ASSERT(file_in.is_open());

        mpz_t bot;
        mpz_init(bot);
        mpz_set_ui(bot, 0u);

        mpz_t top;
        mpz_init_set(top, combinations);

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

            // symbol_bot_scaled = remaining_combinations * symbol_bot / total_number_of_symbols
            mpz_t symbol_bot_scaled;
            mpz_init_set(symbol_bot_scaled, remaining_combinations);
            mpz_mul_ui(symbol_bot_scaled, symbol_bot_scaled, symbol_bot);
            mpz_div_ui(symbol_bot_scaled, symbol_bot_scaled, total_number_of_symbols); // TODO hopefully this leaves no remainder

            // symbol_count_scaled = remaining_combinations * symbol_count / total_number_of_symbols
            mpz_t symbol_count_scaled;
            mpz_init_set(symbol_count_scaled, remaining_combinations);
            mpz_mul_ui(symbol_count_scaled, symbol_count_scaled, symbol_count);
            mpz_div_ui(symbol_count_scaled, symbol_count_scaled, total_number_of_symbols); // TODO hopefully this leaves no remainder

            mpz_add(bot, bot, symbol_bot_scaled);
            mpz_add(top, bot, symbol_count_scaled);

            mpz_clear(remaining_combinations);
            mpz_clear(symbol_bot_scaled);
            mpz_clear(symbol_count_scaled);

        }

        // TODO test and see if we are to add all scaled symbols, are we going to get `combinations`

        cout << "reading encoded data..." << endl;

        {
            FILE* file_out = fopen(file_compressed_str.c_str(), "wb");
            ASSERT(file_out);

            mpz_out_raw(file_out, bot);

            fclose(file_out);
        }

    }

    cout << "decoding..." << endl;

    {
        mpz_t num;
        mpz_init(num);

        cout << "reading encoded data..." << endl;

        {
            FILE* file_in = fopen(file_compressed_str.c_str(), "rb");
            ASSERT(file_in);

            mpz_inp_raw(num, file_in);

            fclose(file_in);
        }

        cout << "decoding..." << endl;

        {
            ofstream file_out;
            file_out.open(file_regenerated_str, ios::binary);
            ASSERT(file_out.is_open());

            mpz_t bot;
            mpz_init_set_ui(bot, 0u);

            mpz_t top;
            mpz_init_set(top, combinations);

            for(uint32_t i = 0; i < total_number_of_symbols - PADDING; ++i){ // tuk ima 1 edge case v koito toq look da stane infinite
                
                // remaining_combinations = top - bot
                mpz_t remaining_combinations;
                mpz_init_set(remaining_combinations, top);
                mpz_sub(remaining_combinations, remaining_combinations, bot);

                bool symbol_found = false;

                for(size_t symbol_s = 0; symbol_s < symbol_counts.size(); ++symbol_s){

                    uint32_t symbol_count = symbol_counts.at(symbol_s);
                    uint32_t symbol_bot = symbol_bots.at(symbol_s);

                    // symbol_bot_scaled = remaining_combinations * symbol_bot / total_number_of_symbols
                    mpz_t symbol_bot_scaled;
                    mpz_init_set(symbol_bot_scaled, remaining_combinations);
                    mpz_mul_ui(symbol_bot_scaled, symbol_bot_scaled, symbol_bot);
                    mpz_div_ui(symbol_bot_scaled, symbol_bot_scaled, total_number_of_symbols); // TODO hopefully this leaves no remainder

                    // symbol_count_scaled = remaining_combinations * symbol_count / total_number_of_symbols
                    mpz_t symbol_count_scaled;
                    mpz_init_set(symbol_count_scaled, remaining_combinations);
                    mpz_mul_ui(symbol_count_scaled, symbol_count_scaled, symbol_count);
                    mpz_div_ui(symbol_count_scaled, symbol_count_scaled, total_number_of_symbols); // TODO hopefully this leaves no remainder

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

    }

    mpz_clear(combinations);

    return 0;
}
