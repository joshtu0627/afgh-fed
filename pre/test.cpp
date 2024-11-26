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

#include <chrono>
#include <gmpxx.h>
#include <iostream>

#include "pre-hom.h"
#include "test_pre_udf.h"

extern "C" {
#include "pre-afgh-relic.h"
}

using namespace std::chrono;

#define FP_BYTES RLC_FP_BYTES
#define FP_STR FP_BYTES * 2 + 1
#define G1_LEN (FP_BYTES * 2) + 2
#define G2_LEN (FP_BYTES * 4) + 4
#define GT_LEN (FP_BYTES * 12) + 12
#define BN_NEG RLC_NEG
#define BN_POS RLC_POS
#define STS_OK RLC_OK
#define G1_TABLE RLC_G1_TABLE
#define G2_TABLE RLC_G2_TABLE
#define CMP_GT RLC_GT
#define CMP_EQ RLC_EQ
#define BN_BYTES (RLC_BN_DIGS * sizeof(dig_t))
#define fp_write fp_write_str


int basic_test() {
    uint64_t msg1=23421345, msg2=50;
    pre_keys_t alice_key, bob_key;
    pre_ciphertext_t alice_cipher1, alice_cipher2, alice_add, bob_re;
    pre_re_token_t token_to_bob;
    dig_t res;

    pre_generate_keys(alice_key);	
	
/////////////	
/*//-
  pre_params_t params;
  pre_sk_t alice_sk, bob_sk;
  pre_pk_t alice_pk, bob_pk;
  pre_token_t token_to_bob;
  pre_plaintext_t plaintext, decrypted;
  pre_ciphertext_t cipher;
  pre_re_ciphertext_t re_cipher;
  uint8_t key1[16];
  uint8_t key2[16];
  int ok = 1;

  // generate random message
  pre_rand_plaintext(plaintext);

  pre_generate_params(params);
  pre_generate_sk(alice_sk, params);
  pre_derive_pk(alice_pk, params, alice_sk);
  pre_generate_sk(bob_sk, params);
  pre_derive_pk(bob_pk, params, bob_sk);

  pre_encrypt(cipher, params, alice_pk, plaintext);

  pre_decrypt(decrypted, params, alice_sk, cipher);

  if (gt_cmp(plaintext->msg, decrypted->msg) == RLC_EQ) {
    std::cout << "Encrypt decrypt OK!" << std::endl;
  } else {
    std::cout << "Encrypt decrypt failed!" << std::endl;
  }

  pre_generate_token(token_to_bob, params, alice_sk, bob_pk);

  pre_apply_token(re_cipher, token_to_bob, cipher);

  pre_decrypt_re(decrypted, params, bob_sk, re_cipher);

  if (gt_cmp(plaintext->msg, decrypted->msg) == RLC_EQ) {
    std::cout << "Re-encrypt decrypt OK!" << std::endl;
  } else {
    std::cout << "Re-encrypt decrypt failed!" << std::endl;
  }

  pre_map_to_key(key1, 16, decrypted);
  pre_map_to_key(key2, 16, plaintext);

  for (int i = 0; i < 16; i++) {
    if (key1[i] != key2[i]) {
      ok = 0;
      break;
    }
  }

  if (ok) {
    std::cout << "Map to key OK!" << std::endl;
  } else {
    std::cout << "Map to key failed!" << std::endl;
  }
*///-
  return 0;
}


void encode_decode_test() {
/*//-	
  pre_plaintext_t plaintext, plaintext_decoded, decrypted;
  pre_params_t params, params_decoded;
  pre_sk_t alice_sk, bob_sk, alice_sk_decoded;
  pre_pk_t alice_pk, bob_pk, alice_pk_decoded;
  pre_ciphertext_t alice_cipher1, alice_cipher1_decode;
  pre_re_ciphertext_t bob_re, bob_re_decode;
  pre_token_t token_to_bob, token_to_bob_decode;
  int size;
  char *buff;

  pre_rand_plaintext(plaintext);
  size = get_encoded_plaintext_size(plaintext);
  buff = (char *)malloc(size);
  if (!encode_plaintext(buff, size, plaintext) == RLC_OK) {
    std::cout << "Message encode error!" << std::endl;
    exit(1);
  }
  if (!decode_plaintext(plaintext_decoded, buff, size) == RLC_OK) {
    std::cout << "Message decode error!" << std::endl;
    exit(1);
  }
  free(buff);

  if (gt_cmp(plaintext->msg, plaintext_decoded->msg) == RLC_EQ) {
    std::cout << "Decode message OK!" << std::endl;
  } else {
    std::cout << "Decode message Failed!" << std::endl;
  }

  pre_generate_params(params);
  pre_generate_sk(alice_sk, params);
  pre_derive_pk(alice_pk, params, alice_sk);
  pre_generate_sk(bob_sk, params);
  pre_derive_pk(bob_pk, params, bob_sk);
  pre_generate_token(token_to_bob, params, alice_sk, bob_pk);
  pre_encrypt(alice_cipher1, params, alice_pk, plaintext);

  size = get_encoded_params_size(params);
  buff = (char *)malloc(size);
  if (!encode_params(buff, size, params) == RLC_OK) {
    std::cout << "Params encode error!" << std::endl;
    exit(1);
  }
  if (!decode_params(params_decoded, buff, size) == RLC_OK) {
    std::cout << "Params decode error!" << std::endl;
    exit(1);
  }
  free(buff);

  if (gt_cmp(params->Z, params_decoded->Z) == RLC_EQ &&
      g1_cmp(params->g1, params_decoded->g1) == RLC_EQ &&
      g2_cmp(params->g2, params_decoded->g2) == RLC_EQ) {
    std::cout << "Decode params OK!" << std::endl;
  } else {
    std::cout << "Decode params failed!" << std::endl;
  }

  size = get_encoded_sk_size(alice_sk);
  buff = (char *)malloc(size);
  if (!encode_sk(buff, size, alice_sk) == RLC_OK) {
    std::cout << "Secret key encode error!" << std::endl;
    exit(1);
  }
  if (!decode_sk(alice_sk_decoded, buff, size) == RLC_OK) {
    std::cout << "Secret key decode error!" << std::endl;
    exit(1);
  }
  free(buff);

  if (bn_cmp(alice_sk->a, alice_sk_decoded->a) == RLC_EQ &&
      bn_cmp(alice_sk->a_inv, alice_sk_decoded->a_inv) == RLC_EQ) {
    std::cout << "Secret key OK!" << std::endl;
  } else {
    std::cout << "Secret key failed!" << std::endl;
  }

  size = get_encoded_pk_size(alice_pk);
  buff = (char *)malloc(size);
  if (!encode_pk(buff, size, alice_pk) == RLC_OK) {
    std::cout << "Public key encode error!" << std::endl;
    exit(1);
  }
  if (!decode_pk(alice_pk_decoded, buff, size) == RLC_OK) {
    std::cout << "Public key decode error!" << std::endl;
    exit(1);
  }
  free(buff);

  if (g1_cmp(alice_pk->pk1, alice_pk_decoded->pk1) == RLC_EQ &&
      g2_cmp(alice_pk->pk2, alice_pk_decoded->pk2) == RLC_EQ) {
    std::cout << "Decode public key OK!" << std::endl;
  } else {
    std::cout << "Decode public key failed!" << std::endl;
  }

  if (g1_cmp(alice_pk->pk1, alice_pk_decoded->pk1) == RLC_EQ &&
      g2_cmp(alice_pk->pk2, alice_pk_decoded->pk2) == RLC_EQ) {
    std::cout << "Public key OK!" << std::endl;
  } else {
    std::cout << "Public key failed!" << std::endl;
  }

  size = get_encoded_token_size(token_to_bob);
  buff = (char *)malloc(size);
  if (!encode_token(buff, size, token_to_bob) == RLC_OK) {
    std::cout << "Token encode error!" << std::endl;
    exit(1);
  }
  if (!decode_token(token_to_bob_decode, buff, size) == RLC_OK) {
    std::cout << "Token decode error!" << std::endl;
    exit(1);
  }
  free(buff);

  if (g2_cmp(token_to_bob->token, token_to_bob_decode->token) == RLC_EQ) {
    std::cout << "Decode token OK!" << std::endl;
  } else {
    std::cout << "Decode token failed!" << std::endl;
  }

  size = get_encoded_ciphertext_size(alice_cipher1);
  buff = (char *)malloc(size);
  encode_ciphertext(buff, size, alice_cipher1);
  decode_ciphertext(alice_cipher1_decode, buff, size);
  free(buff);

  if (gt_cmp(alice_cipher1->c1, alice_cipher1_decode->c1) == RLC_EQ &&
      g1_cmp(alice_cipher1->c2, alice_cipher1_decode->c2) == RLC_EQ) {
    std::cout << "Decode cipher OK!" << std::endl;
  } else {
    std::cout << "Decode cipher failed!" << std::endl;
  }

  pre_apply_token(bob_re, token_to_bob, alice_cipher1);

  size = get_encoded_re_ciphertext_size(bob_re);
  buff = (char *)malloc(size);
  encode_re_ciphertext(buff, size, bob_re);
  decode_re_ciphertext(bob_re_decode, buff, size);
  free(buff);

  if (gt_cmp(bob_re->c1, bob_re_decode->c1) == RLC_EQ &&
      gt_cmp(bob_re->c2, bob_re_decode->c2) == RLC_EQ) {
    std::cout << "Decode re-encrypted cipher OK!" << std::endl;
  } else {
    std::cout << "Decode re-encrypted cipher failed!" << std::endl;
  }
  pre_decrypt(decrypted, params, alice_sk, alice_cipher1);
  if (gt_cmp(decrypted->msg, plaintext->msg) == RLC_EQ) {
    std::cout << "Decrypt OK!" << std::endl;
  } else {
    std::cout << "Dec Failed!" << std::endl;
  }
 *///- 
}

int main() {
  pre_init();
  std::cout << "---- PRE Tests" << std::endl;
  basic_test();
  std::cout << "---- Encode/Decode Tests" << std::endl;
  encode_decode_test();
  //-pre_cleanup();
  return 0;
}
