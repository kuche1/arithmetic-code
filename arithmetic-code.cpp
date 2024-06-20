
#include <iostream>
#include <fstream>
#include <cassert>
#include <climits>
#include <gmp.h>
#include <arpa/inet.h>
#include <vector>
#include <cstdio>
#include <thread>
#include <memory>
#include <iomanip>

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

#define USER_ERR_CANT_OPEN_FILE(path) { \
    USER_ERR("Could not open file `" << path << "`"); \
}

#define SYMBOL_COUNTS_TYPE array<uint16_t, 256>
// we need 1 count for each byte -> 2**8 -> length needs to be 256
// with uint32_t we can count 2**32 -> 4294967296 times each character, so if a file consist all of the same character, the can process at max a  4GiB (4294967296 / 1024 / 1024 / 1024) file
// with uint16_t we can count 2**16 -> 65536      times each character, so if a file consist all of the same character, the can process at max a 64KiB (65536      / 1024 / 1024 / 1024) file

#define SYMBOL_BOTS_TYPE array<uint32_t, 256>

struct fuck_wrapper_around_gmp {

    mpz_t value;

    fuck_wrapper_around_gmp() {
        mpz_init(value);
    }

    ~fuck_wrapper_around_gmp() {
        mpz_clear(value);
    }

    mpz_t & get() {
        return value;
    }

};

SYMBOL_BOTS_TYPE calculate_symbol_bots(const SYMBOL_COUNTS_TYPE & symbol_counts){

    SYMBOL_BOTS_TYPE symbol_bots = {};

    {
        uint32_t base = 0;

        for(size_t symbol_idx = 0; symbol_idx < symbol_counts.size(); ++symbol_idx){
            uint16_t symbol_count = symbol_counts.at(symbol_idx);

            uint32_t bot = base;

            base = base + symbol_count;

            symbol_bots.at(symbol_idx) = bot;
        }
    }

    return symbol_bots;

}

void combine_files_and_delete(const string & sum, const vector<string> & blocks){

    ofstream file_out;
    file_out.open(sum, ios::binary);
    ASSERT(file_out.is_open());

    for(const string & block : blocks){

        {

            ifstream file_in;
            file_in.open(block, ios::binary);
            ASSERT(file_in.is_open());

            char buffer[1]; // TODO da go eba tova laino, ako sloja ne6to pove4e ot 1 qvno endianness-a se naebava

            while(true){

                file_in.read(buffer, sizeof(buffer));

                size_t bytes_read = file_in.gcount();

                if(bytes_read <= 0){
                    break;
                }

                file_out.write(buffer, sizeof(buffer));

            }

        }

        ASSERT(remove(block.c_str()) == 0);

    }

}

size_t get_file_size(const string & file){

    ifstream file_in;
    file_in.open(file, ios::binary | ios::ate);
    ASSERT(file_in.is_open());

    return file_in.tellg();

}

uint32_t calculate_total_number_of_symbols(const SYMBOL_COUNTS_TYPE & symbol_counts){

    uint32_t total_number_of_symbols = 0;

    {
        for(uint16_t count : symbol_counts){
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

    for(uint16_t count : symbol_counts){
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

void encode_block(const string & file_to_compress, size_t start_pos, size_t block_size, const string & file_compressed){

    // cout << "counting symbols..." << endl;

    SYMBOL_COUNTS_TYPE symbol_counts = {};

    {

        ifstream file_in;
        file_in.open(file_to_compress, ios::binary);
        ASSERT(file_in.is_open());

        file_in.seekg(start_pos, ios::beg);

        for(size_t char_idx = 0; char_idx < block_size; ++char_idx){

            unsigned char symbol;
            file_in.read(reinterpret_cast<char*>(&symbol), 1);

            if(file_in.gcount() <= 0){
                break;
            }

            symbol_counts.at(symbol) += 1;
            ASSERT(symbol_counts.at(symbol) > 0);
        }

    }

    // cout << "writing symbol counts..." << endl;

    {
        ofstream file_out;
        file_out.open(file_compressed, ios::binary);
        ASSERT(file_out.is_open());

        for(uint16_t count : symbol_counts){
            uint16_t big_endian = htons(count);
            file_out.write(reinterpret_cast<char *>(&big_endian), sizeof(big_endian));
        }
    }

    // cout << "calculating ranges..." << endl;

    SYMBOL_BOTS_TYPE symbol_bots = calculate_symbol_bots(symbol_counts);

    // cout << "calculating number of symbols..." << endl;

    uint32_t total_number_of_symbols = calculate_total_number_of_symbols(symbol_counts);

    // cout << "calculating possible combinations..." << endl;

    mpz_t combinations;
    mpz_init(combinations);
    calculate_possible_combinations(combinations, symbol_counts, total_number_of_symbols);

    // cout << "encoding..." << endl;

    mpz_t bot;
    mpz_init(bot);
    mpz_set_ui(bot, 0u);

    {

        uint32_t number_of_remaining_symbols = total_number_of_symbols;

        ifstream file_in;
        file_in.open(file_to_compress, ios::binary);
        ASSERT(file_in.is_open());

        file_in.seekg(start_pos, ios::beg);

        mpz_t top;
        mpz_init_set(top, combinations);

        mpz_clear(combinations);

        for(size_t char_idx = 0; char_idx < block_size; ++char_idx){

            unsigned char symbol;
            file_in.read(reinterpret_cast<char*>(&symbol), 1);

            if(file_in.gcount() <= 0){
                break;
            }

            // remaining_combinations = top - bot
            mpz_t remaining_combinations;
            mpz_init_set(remaining_combinations, top);
            mpz_sub(remaining_combinations, remaining_combinations, bot);

            uint16_t symbol_count = symbol_counts.at(symbol);
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

    // cout << "writing encoded data..." << endl;

    {
        FILE* file_out = fopen(file_compressed.c_str(), "ab");
        ASSERT(file_out);

        mpz_out_raw(file_out, bot);

        fclose(file_out);
    }

}

void decode_block(SYMBOL_COUNTS_TYPE symbol_counts, shared_ptr<fuck_wrapper_around_gmp> num, const string & file_regenerated){

    // cout << "calculating symbol ranges..." << endl;

    SYMBOL_BOTS_TYPE symbol_bots = calculate_symbol_bots(symbol_counts);

    // cout << "calculating number of symbols..." << endl;

    uint32_t total_number_of_symbols = calculate_total_number_of_symbols(symbol_counts);

    // cout << "calculating possible combinations..." << endl;

    mpz_t combinations;
    mpz_init(combinations);
    calculate_possible_combinations(combinations, symbol_counts, total_number_of_symbols);

    // cout << "decoding..." << endl;

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

                uint16_t symbol_count = symbol_counts.at(symbol_s);

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

                if(mpz_cmp(num->get(), new_bot) >= 0){ // num >= new_bot
                    if(mpz_cmp(num->get(), new_top) < 0){ // num < new_top
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

#define TMP_FILE_PREFIX "/var/tmp/arithmetic-code-tmp-" // TODO what if we're running 2 instances of the program

void print_progress(size_t bytes_read, size_t bytes_max, bool waiting_for_last_threads_to_finish, size_t threads, size_t threads_max){

    float bytes_read_percent = 100.0f * static_cast<float>(bytes_read) / static_cast<float>(bytes_max);

    cout << "progress: ";
    cout << bytes_read_percent << "%";
    if(waiting_for_last_threads_to_finish){
        cout << " (waiting for last threads to finish)";
    }else{
        cout << " [bytes read: " << bytes_read << " / " << bytes_max << "]";
    }
    cout << " [threads: " << threads << " / " << threads_max << "]" << endl;

}

bool controll_multithreaded_workload(size_t bytes_read, size_t bytes_max, vector<thread> & threads, vector<atomic<bool> *> & threads_finished, size_t threads_max){

    while(true){

        bool EOF_reached = bytes_read >= bytes_max;

        print_progress(bytes_read, bytes_max, EOF_reached, threads.size(), threads_max);

        if(EOF_reached){

            if(threads.size() <= 0){
                return true;
            }

        }else{

            if(threads.size() < threads_max){
                return false;
            }

        }

        bool at_least_one_thread_freed = false;

        while(true){

            for(ssize_t thr_idx = threads_finished.size()-1; thr_idx >= 0; --thr_idx){
                atomic<bool> * finished = threads_finished[thr_idx];

                if(finished->load()){

                    at_least_one_thread_freed = true;

                    // remove from vector
                    threads_finished[thr_idx] = move(threads_finished.back());
                    threads_finished.pop_back();

                    // free mem
                    delete finished;

                    // free mem
                    threads[thr_idx].join();

                    // remove from vector
                    threads[thr_idx] = move(threads.back());
                    threads.pop_back();
                }
            }

            if(at_least_one_thread_freed){
                break;
            }else{
                this_thread::sleep_for(chrono::milliseconds(35));
            }

        }

    }

}

void encode_multithreaded(const string & file_to_compress, const string & file_compressed, uint32_t block_size, size_t threads_max){

    size_t file_size = get_file_size(file_to_compress);

    vector<string> blocks = {};

    vector<thread> threads = {};
    vector<atomic<bool> *> threads_finished = {};

    size_t start = 0z;

    for(size_t iter = 0z;; ++iter){

        {

            bool end = controll_multithreaded_workload(start, file_size, threads, threads_finished, threads_max);

            if(end){
                break;
            }
            
        }

        {

            string tmp_file = TMP_FILE_PREFIX + to_string(iter);
            blocks.push_back(tmp_file);

            atomic<bool> * thread_done = new atomic<bool>(false);
            threads_finished.push_back(thread_done);

            threads.push_back(
                thread(
                    [file_to_compress, start, tmp_file, thread_done, block_size](){
                        encode_block(file_to_compress, start, block_size, tmp_file);
                        thread_done->store(true);
                    }
                )
            );

        }

        start += block_size;

    }

    cout << "assembling file..." << endl;

    combine_files_and_delete(file_compressed, blocks);

    cout << "done" << endl;
}

void decode_multithreaded(const string & file_compressed, const string & file_regenerated, size_t threads_max){

    size_t file_size = get_file_size(file_compressed);

    FILE * file_in = fopen(file_compressed.c_str(), "rb");
    if(!file_in){
        USER_ERR_CANT_OPEN_FILE(file_compressed);
    }

    vector<string> blocks = {};

    vector<thread> threads = {};
    vector<atomic<bool> *> threads_finished = {};

    for(size_t iter = 0;; ++iter){

        {

            long bytes_read = ftell(file_in);
            ASSERT(bytes_read >= 0);

            bool end = controll_multithreaded_workload(bytes_read, file_size, threads, threads_finished, threads_max);

            if(end){
                break;
            }
            
        }

        // read header

        SYMBOL_COUNTS_TYPE symbol_counts = {};

        for(uint16_t & count : symbol_counts){

            uint16_t big_endian;
            ASSERT( fread(&big_endian, sizeof(big_endian), 1, file_in) == 1 );

            count = ntohs(big_endian);
        }

        // read data

        shared_ptr<fuck_wrapper_around_gmp> num = make_shared<fuck_wrapper_around_gmp>();

        mpz_init(num->get());

        ASSERT( mpz_inp_raw(num->get(), file_in) != 0);

        // process block

        atomic<bool> * thread_done = new atomic<bool>(false);
        threads_finished.push_back(thread_done);

        string tmp_file = TMP_FILE_PREFIX + to_string(iter);
        blocks.push_back(tmp_file);

        threads.push_back(
            thread(
                [symbol_counts, num, tmp_file, thread_done](){
                    decode_block(symbol_counts, num, tmp_file);
                    thread_done->store(true);
                }
            )
        );

    }

    fclose(file_in);

    cout << "assembling file..." << endl;

    combine_files_and_delete(file_regenerated, blocks);

    cout << "done" << endl;
}

int main(int argc, char * * argv){

    constexpr size_t THREADS_MAX_DEFAULT = 11;
    constexpr size_t ENCODER_BLOCK_SIZE = 10240; // in bytes

    const string ACTION_ENCODE = "enc";
    const string ACTION_DECODE = "dec";

    // if we are to print any floats, print them with 2 chars of precision
    cout << fixed << setprecision(2);

    if(argc != 4){
        USER_ERR(
            endl <<
            "    You need to provice exactly 3 arguments:" << endl <<
            "        action (`" << ACTION_ENCODE << "` to encode/compress, or `" << ACTION_DECODE << "` to decode/decompress)" << endl <<
            "        input file" << endl <<
            "        output file" << endl
        );
    }

    string action = argv[1];
    string file_in = argv[2];
    string file_out = argv[3];

    if(action == ACTION_ENCODE){

        encode_multithreaded(file_in, file_out, ENCODER_BLOCK_SIZE, THREADS_MAX_DEFAULT);

    }else if(action == ACTION_DECODE){

        decode_multithreaded(file_in, file_out, THREADS_MAX_DEFAULT);

    }else{

        USER_ERR(
            endl <<
            "    Invalid action `" << action << "`" << endl <<
            "    Only valid actions are `" << ACTION_ENCODE << "` and `" << ACTION_DECODE << "`"
        );

    }

    return 0;
}
