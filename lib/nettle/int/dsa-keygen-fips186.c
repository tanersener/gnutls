/* dsa-keygen.c
 *
 * Generation of DSA keypairs
 */

/* nettle, low-level cryptographics library
 *
 * Copyright (C) 2013, 2014 Red Hat
 *  
 * The nettle library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 * 
 * The nettle library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with the nettle library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02111-1301, USA.
 */

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <nettle/dsa.h>
#include <dsa-fips.h>

#include <nettle/bignum.h>

unsigned _dsa_check_qp_sizes(unsigned q_bits, unsigned p_bits)
{
	switch (q_bits) {
	case 160:
		if (p_bits != 1024)
			return 0;
		break;
	case 224:
		if (p_bits != 2048)
			return 0;
		break;
	case 256:
		if (p_bits != 2048 && p_bits != 3072)
			return 0;
		break;
	default:
		return 0;
	}
	return 1;
}

/* This generates p,q params using the algorithm in FIPS 186-4.
 * 
 * The hash function used is SHA384.
 */
int
_dsa_generate_dss_pq(struct dsa_public_key *pub,
		     struct dss_params_validation_seeds *cert,
		     unsigned seed_length, void *seed,
		     void *progress_ctx, nettle_progress_func * progress,
		     unsigned p_bits /* = L */ , unsigned q_bits /* = N */ )
{
	mpz_t r, p0, t, z, s, tmp, dp0;
	int ret;
	unsigned iterations, old_counter, i;
	uint8_t *storage = NULL;
	unsigned storage_length;

	ret = _dsa_check_qp_sizes(q_bits, p_bits);
	if (ret == 0)
		return 0;

	if (seed_length < q_bits / 8) {
		return 0;
	}

	mpz_init(p0);
	mpz_init(dp0);
	mpz_init(r);
	mpz_init(t);
	mpz_init(z);
	mpz_init(s);
	mpz_init(tmp);

	nettle_mpz_set_str_256_u(s, seed_length, seed);

	/* firstseed < 2^(N-1) */
	mpz_set_ui(r, 1);
	mpz_mul_2exp(r, r, q_bits - 1);

	if (mpz_cmp(s, r) < 0) {
		goto fail;
	}

	cert->qseed_length = sizeof(cert->qseed);
	cert->pseed_length = sizeof(cert->pseed);

	ret = st_provable_prime(pub->q,
				&cert->qseed_length, cert->qseed,
				&cert->qgen_counter,
				q_bits,
				seed_length, seed, progress_ctx, progress);
	if (ret == 0) {
		goto fail;
	}

	if (progress)
		progress(progress_ctx, 'q');

	ret = st_provable_prime(p0,
				&cert->pseed_length, cert->pseed,
				&cert->pgen_counter,
				1 + ((p_bits + 1) / 2),
				cert->qseed_length, cert->qseed,
				progress_ctx, progress);
	if (ret == 0) {
		goto fail;
	}

	iterations = ((p_bits + DIGEST_SIZE - 1) / DIGEST_SIZE) - 1;
	old_counter = cert->pgen_counter;

	storage_length = iterations * DIGEST_SIZE;
	storage = malloc(storage_length);
	if (storage == NULL)
		goto fail;

	nettle_mpz_set_str_256_u(s, cert->pseed_length, cert->pseed);
	for (i = 0; i < iterations; i++) {
		cert->pseed_length = nettle_mpz_sizeinbase_256_u(s);
		nettle_mpz_get_str_256(cert->pseed_length, cert->pseed, s);

		hash(&storage[(iterations - i - 1) * DIGEST_SIZE],
		     cert->pseed_length, cert->pseed);
		mpz_add_ui(s, s, 1);
	}
	mpz_add_ui(s, s, 1);

	cert->pseed_length = nettle_mpz_sizeinbase_256_u(s);
	nettle_mpz_get_str_256(cert->pseed_length, cert->pseed, s);

	/* x = 2^(p_bits-1) + (x mod 2^(p_bits-1)) */
	nettle_mpz_set_str_256_u(tmp, storage_length, storage);

	mpz_set_ui(r, 1);
	mpz_mul_2exp(r, r, p_bits - 1);

	mpz_mod_2exp(tmp, tmp, p_bits - 1);
	mpz_add(tmp, tmp, r);

	/* Generate candidate prime p in [2^(bits-1), 2^bits] */

	/* t = u[x/2c0] */
	mpz_mul_2exp(dp0, p0, 1);	/* dp0 = 2*p0 */
	mpz_mul(dp0, dp0, pub->q);	/* dp0 = 2*p0*q */

	mpz_cdiv_q(t, tmp, dp0);

 retry:
	/* c = 2p0*q*t + 1 */
	mpz_mul(pub->p, dp0, t);
	mpz_add_ui(pub->p, pub->p, 1);

	if (mpz_sizeinbase(pub->p, 2) > p_bits) {
		/* t = 2^(bits-1)/2qp0 */
		mpz_set_ui(tmp, 1);
		mpz_mul_2exp(tmp, tmp, p_bits - 1);
		mpz_cdiv_q(t, tmp, dp0);

		/* p = t* 2tq p0 + 1 */
		mpz_mul(pub->p, dp0, t);
		mpz_add_ui(pub->p, pub->p, 1);
	}

	cert->pgen_counter++;

	for (i = 0; i < iterations; i++) {
		cert->pseed_length = nettle_mpz_sizeinbase_256_u(s);
		nettle_mpz_get_str_256(cert->pseed_length, cert->pseed, s);

		hash(&storage[(iterations - i - 1) * DIGEST_SIZE],
		     cert->pseed_length, cert->pseed);
		mpz_add_ui(s, s, 1);
	}
	mpz_add_ui(s, s, 1);

	cert->pseed_length = nettle_mpz_sizeinbase_256_u(s);
	nettle_mpz_get_str_256(cert->pseed_length, cert->pseed, s);

	/* r = a */
	nettle_mpz_set_str_256_u(r, storage_length, storage);

	/* a = 2 + (a mod (p-3)) */
	mpz_sub_ui(tmp, pub->p, 3);	/* c is too large to worry about negatives */
	mpz_mod(r, r, tmp);
	mpz_add_ui(r, r, 2);

	/* z = a^(2tq) mod p */
	mpz_mul_2exp(tmp, t, 1);	/* tmp = 2t */
	mpz_mul(tmp, t, pub->q);	/* tmp = 2tq */
	mpz_powm(z, r, tmp, pub->p);

	mpz_sub_ui(tmp, z, 1);

	mpz_gcd(tmp, tmp, pub->p);
	if (mpz_cmp_ui(tmp, 1) == 0) {
		mpz_powm(tmp, z, p0, pub->p);
		if (mpz_cmp_ui(tmp, 1) == 0) {
			goto success;
		}
	}

	if (progress)
		progress(progress_ctx, 'x');

	if (cert->pgen_counter >= (4 * p_bits + old_counter))
		return 0;

	mpz_add_ui(t, t, 1);
	goto retry;

 success:
	if (progress)
		progress(progress_ctx, 'p');

	ret = 1;
	goto finish;

 fail:
	ret = 0;

 finish:
	mpz_clear(dp0);
	mpz_clear(p0);
	mpz_clear(tmp);
	mpz_clear(t);
	mpz_clear(z);
	mpz_clear(s);
	mpz_clear(r);
	free(storage);
	return ret;
}

int
_dsa_generate_dss_g(struct dsa_public_key *pub,
		    struct dss_params_validation_seeds *cert,
		    unsigned first_seed_length, const void *first_seed,
		    void *progress_ctx, nettle_progress_func * progress,
		    unsigned index)
{
	mpz_t e, w;
	uint16_t count;
	uint8_t *dseed = NULL;
	unsigned dseed_size;
	unsigned pos;
	uint8_t digest[DIGEST_SIZE];
	int ret;

	if (index != 1 && index != 2)
		return 0;

	dseed_size =
	    first_seed_length + cert->pseed_length + cert->qseed_length + 4 +
	    1 + 2;
	dseed = malloc(dseed_size);
	if (dseed == NULL)
		return 0;

	mpz_init(e);
	mpz_init(w);

	memcpy(dseed, first_seed, first_seed_length);
	pos = first_seed_length;

	memcpy(dseed + pos, cert->pseed, cert->pseed_length);
	pos += cert->pseed_length;

	memcpy(dseed + pos, cert->qseed, cert->qseed_length);
	pos += cert->qseed_length;

	memcpy(dseed + pos, "\x67\x67\x65\x6e", 4);
	pos += 4;

	*(dseed + pos) = (uint8_t) index;
	pos += 1;

	mpz_sub_ui(e, pub->p, 1);
	mpz_fdiv_q(e, e, pub->q);

	for (count = 1; count < 65535; count++) {
		*(dseed + pos) = (count >> 8) & 0xff;
		*(dseed + pos + 1) = count & 0xff;

		hash(digest, dseed_size, dseed);

		nettle_mpz_set_str_256_u(w, DIGEST_SIZE, digest);

		mpz_powm(pub->g, w, e, pub->p);

		if (mpz_cmp_ui(pub->g, 2) < 0) {
			if (progress)
				progress(progress_ctx, 'x');
			continue;
		}

		/* found */
		goto success;
	}

	/* if we're here we failed */
	if (progress)
		progress(progress_ctx, 'X');
	ret = 0;
	goto finish;

 success:
	if (progress)
		progress(progress_ctx, 'g');

	ret = 1;

 finish:
	free(dseed);
	mpz_clear(e);
	mpz_clear(w);
	return ret;

}

/* Generates the public and private DSA (or DH) keys
 */
void
_dsa_generate_dss_xy(struct dsa_public_key *pub,
		     struct dsa_private_key *key,
		     void *random_ctx, nettle_random_func * random)
{
	mpz_t r;

	mpz_init(r);
	mpz_set(r, pub->q);
	mpz_sub_ui(r, r, 2);
	nettle_mpz_random(key->x, random_ctx, random, r);
	mpz_add_ui(key->x, key->x, 1);

	mpz_powm(pub->y, pub->g, key->x, pub->p);

	mpz_clear(r);
}

/* This generates p, q, g params using the algorithms from FIPS 186-4.
 * For p, q, the Shawe-Taylor algorithm is used.
 * For g, the verifiable canonical generation of the generator is used.
 * 
 * The hash function used is SHA384.
 * 
 * pub: The output public key
 * key: The output private key
 * cert: A certificate that can be used to verify the generated parameters
 * index: 1 for digital signatures (DSA), 2 for key establishment (DH)
 * p_bits: The requested size of p
 * q_bits: The requested size of q
 * 
 */
int
dsa_generate_dss_keypair(struct dsa_public_key *pub,
			 struct dsa_private_key *key,
			 struct dss_params_validation_seeds *cert,
			 unsigned index,
			 void *random_ctx, nettle_random_func * random,
			 void *progress_ctx, nettle_progress_func * progress,
			 unsigned p_bits /* = L */ , unsigned q_bits /* = N */ )
{
	int ret;

	ret = _dsa_check_qp_sizes(q_bits, p_bits);
	if (ret == 0)
		return 0;

	cert->seed_length = (q_bits / 8) + 1;
	random(random_ctx, cert->seed_length, cert->seed);

	ret = _dsa_generate_dss_pq(pub, cert, cert->seed_length, cert->seed,
				   progress_ctx, progress, p_bits, q_bits);
	if (ret == 0)
		return 0;

	ret = _dsa_generate_dss_g(pub, cert, cert->seed_length, cert->seed,
				  progress_ctx, progress, index);
	if (ret == 0)
		return 0;

	_dsa_generate_dss_xy(pub, key, random_ctx, random);

	if (progress)
		progress(progress_ctx, '\n');

	return 1;

}
