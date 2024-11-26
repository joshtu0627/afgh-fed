/*
 * Copyright (c) 2016, Institute for Pervasive Computing, ETH Zurich.
 * All rights reserved.
 *
 * Author:
 *       Lukas Burkhalter <lubu@student.ethz.ch>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>
#include <sstream>
#include <gmpxx.h>
#include <chrono>
#include <cmath>
#include <stdio.h>
#include <vector>
#include <fstream>
#include <iomanip>
#include <limits>

#include <time.h>

#define HEIGHT 800
#define WIDTH 800

#define MAXLINE 1024 * 25

// #include "pre-hom.h"
// #include "pre-relic-udf.h"

extern "C"
{
#include "pre-hom.h"
    // #include "test_pre_udf.h"
    // #include "pre-relic-udf.h"
}

using namespace std::chrono;
using namespace std;

#define FP_BYTES RLC_FP_BYTES
#define FP_STR FP_BYTES * 2 + 1
#define G1_LEN (FP_BYTES * 2) + 2
#define G2_LEN (FP_BYTES * 4) + 4
#define GT_LEN (FP_BYTES * 12) + 12
#define BN_NEG RLC_NEG
#define BN_POS RLC_POS
#define STS_OK RLC_OK
#define STS_ERR RLC_ERR
#define G1_TABLE RLC_G1_TABLE
#define G2_TABLE RLC_G2_TABLE
#define CMP_GT RLC_GT
#define CMP_EQ RLC_EQ
#define BN_BYTES (RLC_BN_DIGS * sizeof(dig_t))
#define fp_write fp_write_str

int g_DPNoiseMatrix_1[800][800];
int g_DPNoiseMatrix_2[800][800];
int g_DPNoiseMatrix_3[800][800];
int g_iLittlestNum = 0;
// int g_DPNoiseMatrix_SpeedUpVerify_1[800];
// int g_DPNoiseMatrix_SpeedUpVerify_2[800];
// int g_DPNoiseMatrix_SpeedUpVerify_3[800];
// int g_DPNoiseMatrix_All_SumByAddDirectly[800][800];

//================================================================
static int USE_HARDCODED_CRT_SIZE = -1;
//
// User #includes go here
//

typedef struct
{
    uint16_t is_first;
    uint16_t num_ciphers;
    pre_ciphertext_t *cur_sum;
    char group;
} sum_state;

void print_bytes(const char *pBytes, const uint32_t nBytes)
{
    char cur;
    int i;
    for (uint32_t j = 0; j < nBytes; j++)
    {
        cur = pBytes[j];
        for (i = 0; i < 8; i++)
        {
            printf("%d", !!((cur << i) & 0x80));
        }
        printf(" ");
    }
    printf("\n");
}

void init_sum_state(sum_state *state, int num_ciphers, char group)
{
    int iter = 0;
    state->is_first = 0;
    state->num_ciphers = num_ciphers;
    state->group = group;
    state->cur_sum = (pre_ciphertext_t *)malloc(num_ciphers * sizeof(pre_ciphertext_t));
    for (; iter < num_ciphers; iter++)
    {
        gt_new((state->cur_sum)[iter]->C1);
        gt_set_unity((state->cur_sum)[iter]->C1);
        if (group == PRE_REL_CIPHERTEXT_IN_G_GROUP)
        {
            g1_new((state->cur_sum)[iter]->C2_G1);
            g1_set_infty((state->cur_sum)[iter]->C2_G1);
        }
        else
        {
            gt_new((state->cur_sum)[iter]->C2_GT);
            gt_set_unity((state->cur_sum)[iter]->C2_GT);
        }
        (state->cur_sum)[iter]->group = group;
    }
}

void reset_points_sum_state(sum_state *state)
{
    int iter = 0, num_ciphers = (state->num_ciphers);
    for (; iter < num_ciphers; iter++)
    {
        gt_set_unity((state->cur_sum)[iter]->C1);
        if ((state->cur_sum)[iter]->group == PRE_REL_CIPHERTEXT_IN_G_GROUP)
        {
            g1_set_infty((state->cur_sum)[iter]->C2_G1);
        }
        else
        {
            gt_set_unity((state->cur_sum)[iter]->C2_GT);
        }
    }
}

void free_cipher_array(pre_ciphertext_t *cipher, int num_ciphers)
{
    int iter = 0;
    for (; iter < num_ciphers; iter++)
    {
        pre_ciphertext_clear(cipher[iter]);
    }
    free(cipher);
}

void free_sum_state(sum_state *state)
{
    if (state->num_ciphers > 0)
    {
        free_cipher_array(state->cur_sum, state->num_ciphers);
    }
    free(state);
}

int decode_ciphers(char *encoded, size_t encoded_len, pre_ciphertext_t **cipher, int *num_ciphers)
{
    // hack
    int iter = 0;
    char *cur_position = encoded;
    int cur_len = 0, temp;
    if (USE_HARDCODED_CRT_SIZE <= 0)
    {
        *num_ciphers = (int)encoded[0];
        cur_position = encoded + 1;
    }
    else
    {
        *num_ciphers = USE_HARDCODED_CRT_SIZE;
    }

    *cipher = (pre_ciphertext_t *)malloc((*num_ciphers) * sizeof(pre_ciphertext_t));
    for (; iter < *num_ciphers; iter++)
    {

        if (decode_cipher((*cipher)[iter], cur_position, (int)encoded_len) == STS_ERR)
        {
            return 1;
        }
        temp = get_encoded_cipher_size((*cipher)[iter]);
        cur_len += temp;
        if (cur_len > encoded_len)
        {
            return 1;
        }
        cur_position += temp;
    }
    return 0;
}

int encode_ciphers(char **encoded, size_t *encoded_len, pre_ciphertext_t *cipher, int num_ciphers)
{
    // hack
    int iter = 0;
    char *cur_position;
    int cur_len = 0, temp;

    for (; iter < num_ciphers; iter++)
    {
        cur_len += get_encoded_cipher_size(cipher[iter]);
    }

    if (USE_HARDCODED_CRT_SIZE <= 0)
    {
        cur_len++;
    }

    *encoded = (char *)malloc((size_t)cur_len);

    if (USE_HARDCODED_CRT_SIZE <= 0)
    {
        cur_position = (*encoded) + 1;
        (*encoded)[0] = (char)num_ciphers;
    }
    else
    {
        cur_position = *encoded;
    }

    for (iter = 0; iter < num_ciphers; iter++)
    {
        temp = get_encoded_cipher_size(cipher[iter]);
        if (encode_cipher(cur_position, temp, cipher[iter]) == STS_ERR)
        {
            return 1;
        }
        cur_position += temp;
    }
    *encoded_len = (size_t)cur_len;
    return 0;
}

int PRE_REL_SUM_init(sum_state **state_in)
{
    pre_init();
    *state_in = (sum_state *)malloc(sizeof(sum_state));
    (*state_in)->num_ciphers = 0;
    (*state_in)->is_first = 1;
    return 0;
}

int PRE_REL_SUM_deinit(sum_state **state_in)
{
    sum_state *state = *state_in;
    free_sum_state(state);
    pre_deinit();
    return 0;
}

void PRE_REL_SUM_clear(sum_state **state_in)
{
    sum_state *state = *state_in;
    if ((state->num_ciphers) > ((uint16_t)0))
    {
        reset_points_sum_state(state);
    }
}

// void addCiphers(pre_ciphertext_t *cur, pre_ciphertext_t *updated, int num) {
void addCiphers_1(pre_keys_t keys, pre_ciphertext_t *cur, pre_ciphertext_t *updated, int num)
{
    int iter = 0;

    std::cout << "num:" << num << std::endl;

    for (; iter < num; iter++)
    {
        std::cout << "iter:" << iter << std::endl;
        pre_homo_add(keys, cur[iter], cur[iter], updated[iter], 0);
        // pre_homo_add( cur[iter], updated[iter], 0);
    }
}

//@
void addCiphers(pre_ciphertext_t *cur, pre_ciphertext_t *updated, int num)
{
    int iter = 0;
    for (; iter < num; iter++)
    {
        pre_homo_add(NULL, cur[iter], cur[iter], updated[iter], 0);
    }
}

int PRE_REL_SUM_add(char *encoded, size_t encodedLength, sum_state **state_in)
{
    pre_ciphertext_t *in;
    int in_num;
    sum_state *state = *state_in;
    dig_t res4;

    decode_ciphers(encoded, encodedLength, &in, &in_num);

    if (state->is_first)
    {
        init_sum_state(state, in_num, in[0]->group);
    }
    else
    {
        if (in_num != state->num_ciphers)
        {
            // nOK
        }
    }
    addCiphers(state->cur_sum, in, in_num);

    //+
    // pre_decrypt(&res4, NULL, state->cur_sum, 1);
    // std::cout << "res4:" << res4 << std::endl;
    //+
    free_cipher_array(in, in_num);
    return 0;
}

char *PRE_REL_SUM(unsigned long *length, sum_state **state_in)
{
    char *res;
    size_t resSize;
    sum_state *state = *state_in;
    encode_ciphers(&res, &resSize, state->cur_sum, state->num_ciphers);
    *length = (unsigned long)resSize;
    return res;
}

size_t create_cipher_crt(char **temp, uint64_t msg, pre_keys_t key, int num_partitions)
{
    size_t res_size = 0;
    pre_ciphertext_t cipher[num_partitions];
    char *res;
    for (int i = 0; i < num_partitions; i++)
    {
        pre_encrypt(cipher[i], key, msg);
    }
    encode_ciphers(temp, &res_size, cipher, num_partitions);
    for (int i = 0; i < num_partitions; i++)
    {
        pre_cipher_clear(cipher[i]);
    }
    return res_size;
}

size_t create_cipher_crt_level2(char **temp, uint64_t msg, pre_keys_t key, pre_re_token_t token, int num_partitions)
{
    size_t res_size = 0;
    pre_ciphertext_t cipher[num_partitions];
    char *res;
    for (int i = 0; i < num_partitions; i++)
    {
        pre_ciphertext_t temp;
        pre_encrypt(temp, key, msg);
        pre_re_apply(token, cipher[i], temp);
        pre_cipher_clear(temp);
    }
    encode_ciphers(temp, &res_size, cipher, num_partitions);
    for (int i = 0; i < num_partitions; i++)
    {
        pre_cipher_clear(cipher[i]);
    }
    return res_size;
}

void test_udf(int num_add, int use_lvl2, int num_partitions)
{
    sum_state *state;
    char *temp, *res;
    size_t tmp_size;
    unsigned long res_len;
    int iter = 0;
    pre_keys_t key, key_to;
    pre_re_token_t token;

    PRE_REL_SUM_init(&state);

    pre_generate_keys(key);
    pre_generate_keys(key_to);
    pre_generate_re_token(token, key, key_to->pk_2);

    PRE_REL_SUM_clear(&state);

    for (iter = 0; iter < num_add; iter++)
    {
        if (use_lvl2)
        {
            tmp_size = create_cipher_crt_level2(&temp, (uint64_t)iter, key, token, num_partitions);
            //-print_bytes(temp, tmp_size);
            PRE_REL_SUM_add(temp, tmp_size, &state);
            free(temp);
        }
        else
        {
            tmp_size = create_cipher_crt(&temp, (uint64_t)iter, key, num_partitions);
            //-print_bytes(temp, tmp_size);
            PRE_REL_SUM_add(temp, tmp_size, &state);
            free(temp);
        }
    }

    res = PRE_REL_SUM(&res_len, &state);

    PRE_REL_SUM_clear(&state);

    PRE_REL_SUM_deinit(&state);

    //+
    std::cout << "res:" << res << std::endl;
    //+
}

//================================================================
// 四捨五入 取到 小數點第 X 位
double rounding(double num, int index)
{
    bool isNegative = false; // whether is negative number or not

    if (num < 0) // if this number is negative, then convert to positive number
    {
        isNegative = true;
        num = -num;
    }

    if (index >= 0)
    {
        int multiplier;
        multiplier = pow(10, index);
        num = (int)(num * multiplier + 0.5) / (multiplier * 1.0);
    }

    if (isNegative) // if this number is negative, then convert to negative number
    {
        num = -num;
    }

    return num;
}

int ReadDPNoiseFileToMatrix(char *pi_strDPNoiseFileName, int DPFileNum)
{
    char buffer[MAXLINE];
    char *record, *line;
    int i = 0, j = 0;

    // Read 3 DP-noise files into matrices: g_DPNoiseMatrix_1, g_DPNoiseMatrix_2, g_DPNoiseMatrix_3
    double temp_MatValue;

    int temp;
    FILE *fstream = fopen(pi_strDPNoiseFileName, "r");

    if (fstream == NULL)
    {
        printf("\n file opening failed ");
        return -1;
    }

    while (((line = fgets(buffer, sizeof(buffer), fstream)) != NULL) && i <= 800)
    {

        if (buffer[0] == '\n')
        {
            break;
        }

        record = strtok(line, ",");

        j = 0;

        while ((record != NULL) && (j <= 800))
        {
            if (DPFileNum == 1)
            {
                temp_MatValue = atof(record);
                record = strtok(NULL, ",");
                g_DPNoiseMatrix_1[i][j] = rounding(temp_MatValue, 1) * 10; // 四捨五入到小數點第三位 //乘1000倍	 => 平移(shift)
                if (g_DPNoiseMatrix_1[i][j] < g_iLittlestNum)
                    g_iLittlestNum = g_DPNoiseMatrix_1[i][j]; // 更新最小值
            }
            else if (DPFileNum == 2)
            {
                temp_MatValue = atof(record);
                record = strtok(NULL, ",");
                g_DPNoiseMatrix_2[i][j] = rounding(temp_MatValue, 1) * 10; // 四捨五入到小數點第三位 //乘1000倍	 => 平移(shift)
                if (g_DPNoiseMatrix_1[i][j] < g_iLittlestNum)
                    g_iLittlestNum = g_DPNoiseMatrix_1[i][j]; // 更新最小值
            }
            else if (DPFileNum == 3)
            {
                temp_MatValue = atof(record);
                record = strtok(NULL, ",");
                g_DPNoiseMatrix_3[i][j] = rounding(temp_MatValue, 1) * 10; // 四捨五入到小數點第三位 //乘1000倍	 => 平移(shift)
                if (g_DPNoiseMatrix_1[i][j] < g_iLittlestNum)
                    g_iLittlestNum = g_DPNoiseMatrix_1[i][j]; // 更新最小值
            }

            j = j + 1;
        }
        i = i + 1;
    }

    return 0;
}

int PREHomoAdd(uint64_t msg1, uint64_t msg2, uint64_t msg3, int i, int j)
{

    // std::cout << " msg1:" << msg1 << ", msg2:" << msg2 << ", msg3:" << msg3 << ", g_iLittlestNum:" << g_iLittlestNum << std::endl;

    clock_t start, end;

    //====initial======================
    // start = clock(); //log time

    pre_keys_t alice_key, bob_key;
    pre_ciphertext_t alice_cipher1, alice_cipher2, alice_cipher3, alice_add, bob_re;
    pre_re_token_t token_to_bob;
    dig_t res;

    // end = clock();   //log time
    // printf("initial: %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);  //log time

    //====generate_keys======================
    // start = clock(); //log time

    pre_generate_keys(alice_key);

    // end = clock();   //log time
    // printf("pre_generate_keys: %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);  //log time

    pre_generate_keys(bob_key);

    //====init_bsgs_table======================
    // //start = clock(); //log time

    // pre_init_bsgs_table(1L<<18); //pre_deinit()會清空此

    // //end = clock();   //log time
    // //printf("pre_init_bsgs_table: %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);  //log time

    //====pre_encrypt======================
    // start = clock(); //log time

    pre_encrypt(alice_cipher1, alice_key, msg1);

    // end = clock();   //log time
    // printf("pre_encrypt: %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);  //log time

    pre_encrypt(alice_cipher2, alice_key, msg2);
    pre_encrypt(alice_cipher3, alice_key, msg3);

    /*
    pre_decrypt(&res, alice_key, alice_cipher1, 1);

    if(((uint64_t)res)==msg1) {
        std::cout << "pre_decrypt msg1 OK!" << std::endl;
        std::cout << res << std::endl;
    } else {
        std::cout << "pre_decrypt msg1 Failed!" << std::endl;
    }
    */

    //====pre_ciphertext_init======================
    // start = clock(); //log time

    pre_ciphertext_init(alice_add, alice_cipher1->group);

    // end = clock();   //log time
    // printf("pre_ciphertext_init: %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);  //log time

    //====pre_homo_add 1======================
    // start = clock(); //log time

    pre_homo_add(alice_key, alice_add, alice_cipher1, alice_cipher2, 0);

    // end = clock();   //log time
    // printf("pre_homo_add 1: %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);  //log time

    //====pre_homo_add 2======================
    // start = clock(); //log time

    pre_homo_add(alice_key, alice_add, alice_add, alice_cipher3, 0);

    // end = clock();   //log time
    // printf("pre_homo_add 2: %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);  //log time

    /*
    pre_decrypt(&res, alice_key, alice_add, 1);

    if(((uint64_t)res)==(msg1+msg2+msg3)) {
        std::cout << "pre_decrypt msg1+msg2+msg3 OK!" << std::endl;
        std::cout << res << std::endl;
    } else {
        std::cout << "pre_decrypt msg1+msg2+msg3 Failed!" << std::endl;
    }
    */

    //====pre_generate_re_token======================
    // start = clock(); //log time

    pre_generate_re_token(token_to_bob, alice_key, bob_key->pk_2);

    // end = clock();   //log time
    // printf("pre_generate_re_token: %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);  //log time

    //====pre_re_apply======================
    // start = clock(); //log time

    pre_re_apply(token_to_bob, bob_re, alice_add);

    // end = clock();   //log time
    // printf("pre_re_apply: %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);  //log time

    //====pre_decrypt======================
    // start = clock(); //log time

    pre_decrypt(&res, bob_key, bob_re, 1);

    // end = clock();   //log time
    // printf("pre_decrypt: %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);  //log time

    //====compare======================
    // start = clock(); //log time

    if (((uint64_t)res) == (msg1 + msg2 + msg3))
    {
        // if(((uint64_t)res) == msgSum) {
        // std::cout << "(re) pre_decrypt msg1+msg2+msg3 OK!" << std::endl;
        // std::cout << res << std::endl;
        // std::cout << "OK!" << " msg1:" << msg1 << ", msg2:" << msg2 << ", msg3:" << msg3 << ", msgSum:" << msgSum << std::endl;
        // std::cout << "OK!" << std::endl;
        return 0;
    }
    else
    {
        // std::cout << "(re) pre_decrypt msg1+msg2+msg3 Failed!" << std::endl;
        std::cout << "Failed!"
                  << " i:" << i << ", j:" << j << ", msg1:" << msg1 << ", msg2:" << msg2 << ", msg3:" << msg3 << ", g_iLittlestNum:" << g_iLittlestNum << std::endl;
        return -1;
        // std::cout << "Failed!" << std::endl;
    }

    // end = clock();   //log time
    // printf("compare: %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC);  //log time
}

int test_udf_crt()
{
    uint64_t msg1 = 7, msg2 = 2;
    pre_keys_t key;
    sum_state *state, *state_out;
    char *encoded;
    size_t enc_len;
    pre_ciphertext_t *cur_sum, *cursumout;
    int num;
    dig_t res1, res2, res3;

    cur_sum = (pre_ciphertext_t *)malloc(2 * sizeof(pre_ciphertext_t));
    pre_generate_keys(key);
    pre_encrypt(cur_sum[0], key, msg1);
    pre_encrypt(cur_sum[1], key, msg2);

    //-pre_decrypt(&res1, key, cur_sum[0], 1);
    //-pre_decrypt(&res2, key, cur_sum[1], 1);

    encode_ciphers(&encoded, &enc_len, cur_sum, 2);
    decode_ciphers(encoded, enc_len, &cursumout, &num);
    addCiphers_1(key, cur_sum, cursumout, 2);
    //@ addCiphers(cur_sum, cursumout, 2);

    pre_decrypt(&res3, key, cur_sum[0], 1);

    std::cout << "res3:" << res3 << std::endl;

    if (((uint64_t)res3) == (msg1 + msg2))
    {
        std::cout << "(2022re) pre_decrypt msg1+msg2 OK!" << std::endl;
        // std::cout << res3 << std::endl;
    }
    else
    {
        std::cout << "(2022re) pre_decrypt msg1+msg2 Failed!" << std::endl;
    }
    // free_sum_state(state);

    return 0;
}

// find the minimum in array
double find_min(vector<double> array)
{
    double min = numeric_limits<double>::max();
    for (int i = 0; i < array.size(); i++)
    {
        if (array[i] < min)
            min = array[i];
    }
    return min;
}

// transfer type double to type unsigned int with the precision
vector<uint> trans_double_to_uint(vector<double> array, double min, int precision)
{
    vector<uint> result(array.size());
    // minus min in order to convert to positive number
    for (int i = 0; i < result.size(); i++)
    {
        double tmp = (array[i] - min) * pow(10, precision);
        // double expression error
        // result[i] = uint(round(tmp * pow(10, precision)) / pow(10, precision));
        result[i] = uint(round(tmp));
    }

    return result;
}

vector<double> trans_uint_to_double(vector<uint> array, vector<double> min, int precision)
{
    double min_sum = 0.0;
    for (int i = 0; i < min.size(); i++)
        min_sum += min[i];

    vector<double> result(array.size());
    // add min back
    for (int i = 0; i < result.size(); i++)
    {
        result[i] = (double(array[i]) * pow(0.1, precision)) + min_sum;
        // double expression error
        result[i] = double(round(result[i] * pow(10, precision)) / pow(10, precision));
    }

    return result;
}

clock_t previous = clock();
int limit = 5 * CLOCKS_PER_SEC;   // 5 seconds a role
void print_loading_bar(int part, int curr, int total, clock_t time)
{
    if (time - previous > limit)
    {
        cout << "loading\tpart" << part << '\t' << curr << " / " << total << endl;
        previous = time;
    }
}

//-test sjw
// pre_init();
// test_udf_crt();
// pre_deinit();

// test_udf(10, 0, 3);
// return 0;
//-

// 執行 ： ./bin/relic_hom
int main()
{
    clock_t start, end;
    start = clock(); // log time

    pre_init();
    cout.precision(7);
    for (int Round = 1; Round <= 3; Round++)
    {
        for (int epsn = 10; epsn <= 18; epsn++)
        {
            cout << "Round: " << Round << endl;
            cout << "EPSN : " << epsn << endl;

            /*============================================parameters======================================================*/
            string dataset = "HeartScale";
            int size = 270;
            int noises_size = pow(size, 2);
            int client_amount = 3;
            int precision = Round; // MVG testing round = precision
            string dataset_path = "./Dataset/" + dataset + "/" + dataset + "_round" + to_string(Round);
            vector<double> minimum = {};
            vector<vector<uint>> noises_trans(client_amount);
            /*============================================parameters======================================================*/

            /*============================================input file======================================================*/
            vector<vector<double>> noises(client_amount);
            for (int i = 0; i < client_amount; i++)
            {
                ifstream input_file(dataset_path + "/mvg_noise_via_affine_tx_"+dataset+"_Part"+to_string(i+1)+"_Epsn"+to_string(epsn)+"_round"+to_string(Round)+".csv", ios::in);
                if (!input_file)
                {
                    cout << "Unable to open " << dataset_path + "/mvg_noise_via_affine_tx_"+dataset+"_Part"+to_string(i+1)+"_Epsn"+to_string(epsn)+"_round"+to_string(Round)+".csv" << endl;
                    return 0;
                }
            
                string line = {};
                while(getline(input_file, line))
                {
                    stringstream ss(line);
                    string tmp = {};
                    while(getline(ss, tmp, ','))
                    {
                        noises[i].push_back(stof(tmp));
                    }
                }
                minimum.push_back(find_min(noises[i]));
                noises_trans[i] = trans_double_to_uint(noises[i], minimum[i], precision);
                input_file.close();
            }
            cout << "input successfully" << endl;
            /*============================================input file======================================================*/

            /*============================================original total======================================================*/
            vector<double> noises_sum_origin = {};
            for (int i = 0; i < noises_size; i++)
            {
                double tmp = 0.0;
                for (int j = 0; j < client_amount; j++)
                {
                    tmp += noises[j][i];
                }
                // double expression error
                tmp = double(round(tmp * pow(10, precision)) / pow(10, precision));
                noises_sum_origin.push_back(tmp);
            }
            ofstream output_origin_file(dataset_path+"/"+dataset+"_Epsn"+to_string(epsn)+"_round"+to_string(Round)+"_SUM_ORIGIN.csv", ios::out);
            if(!output_origin_file)
            {
                cout << "Unable to open output file!" << endl;
                return 0;
            }
            for (int x = 0, i = 0; x < size; x++)
            {
                for (int y = 0; y < size; y++, i++)
                {
                    output_origin_file << noises_sum_origin[i] << ',';
                }
                output_origin_file << '\n';
            }
            output_origin_file.close();
            cout << "original total file output successfully" << endl;
            /*============================================original total======================================================*/

            /*============================================keys generate======================================================*/
            // encrypt noises from clients
            pre_keys_t server_key;
            vector<pre_keys_t> client_keys(client_amount);
            vector<pre_re_token_t> token_to_server(client_amount);

            // generate keys of server, clients and re-encryption
            pre_generate_keys(server_key);
            for (int i = 0; i < client_amount; i++)
            {
                pre_generate_keys(client_keys[i]);                                           // generate client keys
                pre_generate_re_token(token_to_server[i], client_keys[i], server_key->pk_2); // generate re-encryption token
            }
            cout << "key generation done" << endl << endl;
            /*============================================keys generate======================================================*/

            /*==============================================Clients====================================================*/
            // encrypt noises
            cout << "encryption starts" << endl;
            pre_ciphertext_t **noises_trans_en = new pre_ciphertext_t *[client_amount];
            for (int i = 0; i < client_amount; i++)
            {
                noises_trans_en[i] = new pre_ciphertext_t[noises_size];
                for (int j = 0; j < noises_size; j++)
                {
                    pre_encrypt(noises_trans_en[i][j], client_keys[i], noises_trans[i][j]);
                    print_loading_bar(i+1, j, noises_size, clock());
                }
            }
            cout << "encryption ends" << endl << endl;
            /*==============================================Clients====================================================*/

            /*===========================================ProxyServer=======================================================*/
            // re-encrypt noises
            cout << "re-encryption starts" << endl;
            pre_ciphertext_t **noises_trans_re = new pre_ciphertext_t *[client_amount];
            for (int i = 0; i < client_amount; i++)
            {
                noises_trans_re[i] = new pre_ciphertext_t[noises_size];
                for (int j = 0; j < noises_size; j++)
                {
                    pre_re_apply(token_to_server[i], noises_trans_re[i][j], noises_trans_en[i][j]);
                    print_loading_bar(i+1, j, noises_size, clock());
                }
            }
            cout << "re-encryption ends" << endl << endl;

            // add the encrypted noises
            cout << "noises addition starts" << endl;
            // add the sum to noises_trans_re[0]
            pre_ciphertext_t *noises_sum = noises_trans_re[0];
            for (int i = 1; i < client_amount; i++)
            {
                for (int j = 0; j < noises_size; j++)
                {
                    pre_homo_add(server_key, noises_sum[j], noises_sum[j], noises_trans_re[i][j], '0');
                    print_loading_bar(i+1, j, noises_size, clock());
                }
            }
            cout << "noises addition ends" << endl << endl;
            /*===========================================ProxyServer=======================================================*/

            /*==============================================Server====================================================*/
            // decrypt the sum of the noises
            cout << "decryption starts" << endl;
            vector<uint> result_trans(noises_size);
            dig_t res;
            for (int i = 0; i < noises_size; i++)
            {
                pre_decrypt(&res, server_key, noises_sum[i], '1');
                result_trans[i] = (uint)res;
                print_loading_bar(0, i, noises_size, clock());
            }
            cout << "decryption ends" << endl << endl;
            /*==============================================Server====================================================*/

            /*============================================output file======================================================*/
            ofstream output_file(dataset_path+"/"+dataset+"_Epsn"+to_string(epsn)+"_round"+to_string(Round)+"_SUM.csv", ios::out);
            if(!output_file)
            {
                cout << "Unable to open output file!" << endl;
                return 0;
            }
            vector<double> result = {};
            result = trans_uint_to_double(result_trans, minimum, precision);
            for (int x = 0, i = 0; x < size; x++)
            {
                for (int y = 0; y < size; y++, i++)
                {
                    output_file << result[i] << ',';
                }
                output_file << '\n';
            }
            output_file.close();
            cout << "output successfully" << endl << endl;
            /*============================================output file======================================================*/
        }
    }

    end = clock(); // log time

    pre_deinit();

    // 印出結果
    printf("\n Total Time = %f seconds\n", (double)(end - start) / CLOCKS_PER_SEC); // log time

    // 還原原值: 全部加上最小的負數, 然後/1000	(若還需要用原值的話才做)

    return 0;
}

////////////////test/////////////////////////
int basic_test2()
{

    // uint64_t msg1 = (uint64_t) g_DPNoiseMatrix_1[799][799], msg2 = (uint64_t) g_DPNoiseMatrix_2[799][799];

    //+
    // std::cout << msg1 << std::endl;
    // std::cout << msg2 << std::endl;
    //+

    uint64_t msg1 = 1, msg2 = 2;

    pre_keys_t alice_key, bob_key;
    pre_ciphertext_t alice_cipher1, alice_cipher2, alice_add, bob_re;
    pre_re_token_t token_to_bob;
    dig_t res;

    pre_generate_keys(alice_key);
    pre_generate_keys(bob_key);

    pre_init_bsgs_table(1L << 18);

    pre_encrypt(alice_cipher1, alice_key, msg1);
    pre_encrypt(alice_cipher2, alice_key, msg2);

    pre_decrypt(&res, alice_key, alice_cipher1, 1);

    if (((uint64_t)res) == msg1)
    {
        std::cout << "pre_decrypt msg1 OK!" << std::endl;
        std::cout << res << std::endl;
    }
    else
    {
        std::cout << "pre_decrypt msg1 Failed!" << std::endl;
    }

    pre_ciphertext_init(alice_add, alice_cipher1->group);
    pre_homo_add(alice_key, alice_add, alice_cipher1, alice_cipher2, 0);

    pre_decrypt(&res, alice_key, alice_add, 1);

    if (((uint64_t)res) == (msg1 + msg2))
    {
        std::cout << "pre_decrypt msg1+msg2 OK!" << std::endl;
        std::cout << res << std::endl;
    }
    else
    {
        std::cout << "pre_decrypt msg1+msg2 Failed!" << std::endl;
    }

    pre_generate_re_token(token_to_bob, alice_key, bob_key->pk_2);

    // pre_re_apply(token_to_bob, bob_re, alice_cipher1);
    pre_re_apply(token_to_bob, bob_re, alice_add);

    pre_decrypt(&res, bob_key, bob_re, 1);

    // if(((uint64_t)res)==(msg1)) {
    if (((uint64_t)res) == (msg1 + msg2))
    {
        std::cout << "(re) pre_decrypt msg1+msg2 OK!" << std::endl;
        std::cout << res << std::endl;
    }
    else
    {
        std::cout << "(re) pre_decrypt msg1+msg2 Failed!" << std::endl;
    }
}

// Various DEBUG stuff
//  Implements a short benchmark

int short_benchmark(int iter)
{
    uint64_t msgs[iter], res;
    pre_keys_t alice_key, bob_key;
    pre_ciphertext_t alice_ciphers[iter], alice_ciphers_add[iter], bob_ciphers[iter];
    pre_re_token_t token_to_bob;
    double time;
    int ok = 1;

    pre_generate_keys(alice_key);
    pre_generate_keys(bob_key);

    for (int i = 0; i < iter; i++)
    {
        msgs[i] = (uint64_t)i + 1;
    }

    high_resolution_clock::time_point t1 = high_resolution_clock::now();
    for (int i = 0; i < iter; i++)
    {
        pre_encrypt(alice_ciphers[i], alice_key, msgs[i]);
    }
    high_resolution_clock::time_point t2 = high_resolution_clock::now();
    auto ns = duration_cast<nanoseconds>(t2 - t1).count();
    time = (double)ns / iter;
    std::cout << "Enc time is: " << time << " ns" << std::endl;

    for (int i = 0; i < iter; i++)
    {
        pre_decrypt(&res, alice_key, alice_ciphers[i], 0);
        if (res != i + 1)
        {
            ok = 0;
            break;
        }
    }

    if (ok)
    {
        std::cout << "Decryption OK!" << std::endl;
    }
    else
    {
        std::cout << "Decryption Failed :(!" << std::endl;
    }

    t1 = high_resolution_clock::now();
    for (int i = 0; i < iter; i++)
    {
        pre_ciphertext_init(alice_ciphers_add[i], alice_ciphers[0]->group);
        pre_homo_add(alice_key, alice_ciphers_add[i], alice_ciphers[i], alice_ciphers[(i + 1) % iter], 0);
    }
    t2 = high_resolution_clock::now();
    ns = duration_cast<nanoseconds>(t2 - t1).count();
    time = (double)ns / iter;
    std::cout << "Add time per element is: " << time << " ns" << std::endl;

    pre_generate_re_token(token_to_bob, alice_key, bob_key->pk_2);

    t1 = high_resolution_clock::now();
    for (int i = 0; i < iter; i++)
    {
        pre_re_apply(token_to_bob, bob_ciphers[i], alice_ciphers[i]);
    }
    t2 = high_resolution_clock::now();
    ns = duration_cast<nanoseconds>(t2 - t1).count();
    time = (double)ns / iter;
    std::cout << "RE-Enc time is: " << time << " ns" << std::endl;

    ok = 1;
    for (int i = 0; i < iter; i++)
    {
        pre_decrypt(&res, bob_key, bob_ciphers[i], 0);
        if (res != i + 1)
        {
            ok = 0;
            break;
        }
    }

    if (ok)
    {
        std::cout << "Decryption  Bob OK!" << std::endl;
    }
    else
    {
        std::cout << "Decryption Bob Failed :(!" << std::endl;
    }
}

int basic_test()
{

    uint64_t msg1 = 84361.857 * 1000, msg2 = 506048.895 * 1000;
    // uint64_t msg1=1.2, msg2=50;
    pre_keys_t alice_key, bob_key;
    pre_ciphertext_t alice_cipher1, alice_cipher2, alice_add, bob_re;
    pre_re_token_t token_to_bob;
    dig_t res;

    pre_generate_keys(alice_key);
    pre_generate_keys(bob_key);

    pre_init_bsgs_table(1L << 18);

    pre_encrypt(alice_cipher1, alice_key, msg1);
    pre_encrypt(alice_cipher2, alice_key, msg2);

    pre_decrypt(&res, alice_key, alice_cipher1, 1);

    if (((uint64_t)res) == msg1)
    {
        std::cout << "pre_decrypt msg1 OK!" << std::endl;
        std::cout << res << std::endl;
    }
    else
    {
        std::cout << "pre_decrypt msg1 Failed!" << std::endl;
    }

    pre_ciphertext_init(alice_add, alice_cipher1->group);
    pre_homo_add(alice_key, alice_add, alice_cipher1, alice_cipher2, 0);

    pre_decrypt(&res, alice_key, alice_add, 1);

    if (((uint64_t)res) == (msg1 + msg2))
    {
        std::cout << "pre_decrypt msg1+msg2 OK!" << std::endl;
        std::cout << res << std::endl;
    }
    else
    {
        std::cout << "pre_decrypt msg1+msg2 Failed!" << std::endl;
    }

    pre_generate_re_token(token_to_bob, alice_key, bob_key->pk_2);

    // pre_re_apply(token_to_bob, bob_re, alice_cipher1);
    pre_re_apply(token_to_bob, bob_re, alice_add);

    pre_decrypt(&res, bob_key, bob_re, 1);

    // if(((uint64_t)res)==(msg1)) {
    if (((uint64_t)res) == (msg1 + msg2))
    {
        std::cout << "(re) pre_decrypt msg1+msg2 OK!" << std::endl;
        std::cout << res << std::endl;
    }
    else
    {
        std::cout << "(re) pre_decrypt msg1+msg2 Failed!" << std::endl;
    }
}

void encode_decode_test()
{
    uint64_t msg1 = 23421345, msg2 = 50;
    // uint64_t msg1=1.2, msg2=50;
    pre_keys_t alice_key, bob_key, alice_key_decoded;
    pre_ciphertext_t alice_cipher1, alice_cipher1_decode, bob_re, bob_re_decode;
    pre_re_token_t token_to_bob, token_to_bob_decode;
    dig_t res;
    char *buff;
    int key_size;

    pre_generate_keys(alice_key);
    pre_generate_keys(bob_key);
    pre_generate_re_token(token_to_bob, alice_key, bob_key->pk_2);
    pre_encrypt(alice_cipher1, alice_key, msg1);
    key_size = get_encoded_key_size(alice_key);
    buff = (char *)malloc((size_t)key_size);
    if (!encode_key(buff, key_size, alice_key) == RLC_OK)
    {
        std::cout << "Key encode error!" << std::endl;
        exit(1);
    }
    if (!decode_key(alice_key_decoded, buff, key_size) == RLC_OK)
    {
        std::cout << "Key decode error!" << std::endl;
        exit(1);
    }
    free(buff);

    if (bn_cmp(alice_key->sk, alice_key_decoded->sk) == CMP_EQ &&
        gt_cmp(alice_key->Z, alice_key_decoded->Z) == CMP_EQ &&
        g1_cmp(alice_key->pk, alice_key_decoded->pk) == CMP_EQ &&
        g2_cmp(alice_key->pk_2, alice_key_decoded->pk_2) == CMP_EQ &&
        g1_cmp(alice_key->g, alice_key_decoded->g) == CMP_EQ &&
        g2_cmp(alice_key->g2, alice_key_decoded->g2) == CMP_EQ &&
        alice_key->type == alice_key_decoded->type)
    {
        std::cout << "Decode Key OK!" << std::endl;
    }
    else
    {
        std::cout << "Decode Key Failed!" << std::endl;
    }

    key_size = get_encoded_token_size(token_to_bob);
    buff = (char *)malloc((size_t)key_size);
    encode_token(buff, key_size, token_to_bob);
    decode_token(token_to_bob_decode, buff, key_size);
    free(buff);

    if (g2_cmp(token_to_bob->re_token, token_to_bob_decode->re_token) == CMP_EQ)
    {
        std::cout << "Decode Token OK!" << std::endl;
    }
    else
    {
        std::cout << "Decode Token Failed!" << std::endl;
    }

    key_size = get_encoded_cipher_size(alice_cipher1);
    buff = (char *)malloc((size_t)key_size);
    encode_cipher(buff, key_size, alice_cipher1);
    decode_cipher(alice_cipher1_decode, buff, key_size);
    free(buff);

    if (gt_cmp(alice_cipher1->C1, alice_cipher1_decode->C1) == CMP_EQ && g1_cmp(alice_cipher1->C2_G1, alice_cipher1_decode->C2_G1) == CMP_EQ)
    {
        std::cout << "Decode Cipher OK!" << std::endl;
    }
    else
    {
        std::cout << "Decode Cipher Failed!" << std::endl;
    }

    pre_re_apply(token_to_bob, bob_re, alice_cipher1);

    key_size = get_encoded_cipher_size(bob_re);
    buff = (char *)malloc((size_t)key_size);
    encode_cipher(buff, key_size, bob_re);
    decode_cipher(bob_re_decode, buff, key_size);
    free(buff);

    if (gt_cmp(bob_re->C1, bob_re_decode->C1) == CMP_EQ &&
        gt_cmp(bob_re->C2_GT, bob_re_decode->C2_GT) == CMP_EQ)
    {
        std::cout << "Decode Cipher level2 OK!" << std::endl;
    }
    else
    {
        std::cout << "Decode Cipher level2 Failed!" << std::endl;
    }
    pre_init_bsgs_table(1L << 16);
    pre_decrypt(&res, alice_key, alice_cipher1, 1);
    if (res == msg1)
    {
        std::cout << "Dec OK!" << std::endl;
    }
    else
    {
        std::cout << "Dec Failed!" << std::endl;
    }
}

int test_brute()
{
    gt_t G, brute;
    g1_t g1;
    g2_t g2;
    bn_t message, result;

    bn_new(message);
    bn_new(result);
    bn_read_str(message, "1000", 2, 10);

    g1_new(g1);
    g2_new(g2);
    gt_new(G);
    gt_new(brute);

    g1_get_gen(g1);
    g2_get_gen(g2);
    pc_map(G, g1, g2);

    gt_exp(brute, G, message);

    solve_dlog_brute(brute, G, result, 10000);

    if (bn_cmp(message, result) == CMP_EQ)
    {
        std::cout << "OK!" << std::endl;
    }
    else
    {
        std::cout << "Failed!" << std::endl;
    }
}

void write_size(char *buffer, int size)
{
    buffer[0] = (char)((size >> 8) & 0xFF);
    buffer[1] = (char)(size & 0xFF);
}

int read_size(char *buffer)
{
    return ((int)buffer[0] << 8) | buffer[1];
}

/*
int test_udf() {
    uint64_t msg1=0, msg2=2;
    pre_keys_t key;
    sum_state *state,  *state_out;
    char* encoded;
    size_t enc_len;
    pre_ciphertext_t *cur_sum, *cursumout;
    int num;
    dig_t res1,res2,res3;

    state = (sum_state*) malloc(sizeof(sum_state));
    cur_sum = (pre_ciphertext_t*) malloc(sizeof(pre_ciphertext_t));
    pre_generate_keys(key);
    pre_encrypt(cur_sum[0], key, msg1);
    pre_decrypt(&res1, key, cur_sum[0], 1);
    state->num_ciphers=0;
    state->is_first=1;
    init_sum_state(state, 1, PRE_REL_CIPHERTEXT_IN_G_GROUP);

    encode_ciphers(&encoded, &enc_len, cur_sum, 1);
    decode_ciphers(encoded, enc_len, &cursumout, &num);
    pre_decrypt(&res2, key, cursumout[0], 1);

    addCiphers(cur_sum, cursumout, 1);
    pre_decrypt(&res3, key, cur_sum[0], 1);
    free_sum_state(state);



    return 0;
}
*/

/* //- original
int test_udf_crt() {
    uint64_t msg1=0, msg2=2;
    pre_keys_t key;
    sum_state *state,  *state_out;
    char* encoded;
    size_t enc_len;
    pre_ciphertext_t *cur_sum, *cursumout;
    int num;
    dig_t res1,res2,res3;

    cur_sum = (pre_ciphertext_t*) malloc(2 * sizeof(pre_ciphertext_t));
    pre_generate_keys(key);
    pre_encrypt(cur_sum[0], key, msg1);
    pre_encrypt(cur_sum[1], key, msg1);

    encode_ciphers(&encoded, &enc_len, cur_sum, 2);
    decode_ciphers(encoded, enc_len, &cursumout, &num);
    addCiphers(cur_sum, cursumout, 2);

    return 0;
}
*/

int load_int(uint64_t *res, bn_t num)
{
    uint8_t temp[8];
    int iter = 0;
    int size = bn_size_bin(num);
    int diff = 8 - size;
    uint64_t result = 0;
    int start = 0;

    if (size > 8)
    {
        start = size - 8;
    }
    bn_write_bin(temp, size, num);
    for (iter = start; iter < (size + start); iter++)
    {
        int shift = (64 - (8 * (diff + (1 + (iter - start)))));
        result = result | (((uint64_t)temp[iter]) << shift);
    }
    *res = result;
    return RLC_OK;
}

int test_func()
{
    bn_t test;
    uint64_t res;
    bn_new(test);
    bn_set_dig(test, 1L << 54);
    load_int(&res, test);
    std::cout << "is: " << res << " expected: " << (1L << 54) << std::endl;
}

/*//-
int main() {
    pre_init();
    //short_benchmark(1000);
    basic_test();
    //encode_decode_test();
    //test_udf();
    //test_udf_crt();
    //test_func();
    pre_deinit();
    return 0;
}
*/
//-