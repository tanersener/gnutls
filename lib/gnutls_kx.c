/*
 * Copyright (C) 2000,2001 Nikos Mavroyanopoulos
 *
 * This file is part of GNUTLS.
 *
 * GNUTLS is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GNUTLS is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include "gnutls_int.h"
#include "gnutls_handshake.h"
#include "gnutls_kx.h"
#include "gnutls_dh.h"
#include "gnutls_errors.h"
#include "gnutls_algorithms.h"
#include "debug.h"
#include "gnutls_gcry.h"

#define MASTER_SECRET "master secret"
static int generate_normal_master( GNUTLS_STATE state);

int _gnutls_generate_master( GNUTLS_STATE state) {
	if (state->gnutls_internals.resumed==RESUME_FALSE)
		return generate_normal_master(state);
	return 0;
}

static int generate_normal_master( GNUTLS_STATE state) {
int premaster_size;
#ifdef HARD_DEBUG
int i;
#endif
opaque* premaster, *master;
int ret = 0;
char random[2*TLS_RANDOM_SIZE];

	memcpy(random, state->security_parameters.client_random, TLS_RANDOM_SIZE);
	memcpy(&random[TLS_RANDOM_SIZE], state->security_parameters.server_random, TLS_RANDOM_SIZE);

	/* generate premaster */
        premaster_size = state->gnutls_key->key.size;
	premaster = state->gnutls_key->key.data;

#ifdef HARD_DEBUG
	fprintf(stderr, "PREMASTER SECRET[%d]: %s\n", premaster_size, _gnutls_bin2hex(premaster, premaster_size));
	fprintf(stderr, "CLIENT RANDOM[%d]: %s\n", 32, _gnutls_bin2hex(state->security_parameters.client_random,32));
	fprintf(stderr, "SERVER RANDOM[%d]: %s\n", 32, _gnutls_bin2hex(state->security_parameters.server_random,32));
#endif

	if (_gnutls_version_ssl3(state->connection_state.version) == 0) {
		master =
		    gnutls_ssl3_generate_random( premaster, premaster_size,
			       random, 2*TLS_RANDOM_SIZE, TLS_MASTER_SIZE);

	} else {
		master =
		    gnutls_PRF( premaster, premaster_size,
			       MASTER_SECRET, strlen(MASTER_SECRET),
			       random, 2*TLS_RANDOM_SIZE, TLS_MASTER_SIZE); 
	}
	secure_free(premaster);
	state->gnutls_key->key.size = 0;
	state->gnutls_key->key.data = NULL;
	
	if (master==NULL) return GNUTLS_E_MEMORY_ERROR;
	
#ifdef HARD_DEBUG
	fprintf(stderr, "MASTER SECRET: %s\n", _gnutls_bin2hex(master, TLS_MASTER_SIZE));
#endif
	memcpy(state->security_parameters.master_secret, master, TLS_MASTER_SIZE);
	secure_free(master);
	return ret;
}


/* This is called when we want to receive the key exchange message of the
 * server. It does nothing if this type of message is not required
 * by the selected ciphersuite. 
 */
int _gnutls_send_server_kx_message(SOCKET cd, GNUTLS_STATE state)
{
	uint8 *data = NULL;
	int data_size = 0;
	int ret = 0;

	if (state->gnutls_internals.auth_struct->gnutls_generate_server_kx==NULL) 
		return 0;

#ifdef HANDSHAKE_DEBUG
	fprintf(stderr, "Sending server KX message\n");
#endif


	data_size = state->gnutls_internals.auth_struct->gnutls_generate_server_kx( state->gnutls_key, &data);

	if (data_size < 0) {
		gnutls_assert();
		return data_size;
	}

	ret = _gnutls_send_handshake(cd, state, data, data_size, GNUTLS_SERVER_KEY_EXCHANGE);
	gnutls_free(data);

	if (ret < 0) {
		gnutls_assert();
		return ret;
	}
	return data_size;
}

/* Currently only used in SRP */
int _gnutls_send_server_kx_message2(SOCKET cd, GNUTLS_STATE state)
{
	uint8 *data = NULL;
	int data_size = 0;
	int ret = 0;

	if (state->gnutls_internals.auth_struct->gnutls_generate_server_kx2 != NULL) {
		data_size = state->gnutls_internals.auth_struct->gnutls_generate_server_kx2( state->gnutls_key, &data);

#ifdef HANDSHAKE_DEBUG
		fprintf(stderr, "Sending server KX message2\n");
#endif

		if (data_size<0) {
			gnutls_assert();
			return data_size;
		}

		ret = _gnutls_send_handshake(cd, state, data, data_size, GNUTLS_SERVER_KEY_EXCHANGE);
		gnutls_free(data);
		if (ret<0) {
			gnutls_assert();
			return ret;
		}
		
	}
	return data_size;
}

/* This is the function for the client to send the key
 * exchange message 
 */
int _gnutls_send_client_kx_message(SOCKET cd, GNUTLS_STATE state)
{
	uint8 *data;
	int data_size;
	int ret = 0;

	if (state->gnutls_internals.auth_struct->gnutls_generate_client_kx==NULL) 
		return 0;

#ifdef HANDSHAKE_DEBUG
	fprintf(stderr, "Sending client KX message\n");
#endif

	data_size = state->gnutls_internals.auth_struct->gnutls_generate_client_kx( state->gnutls_key, &data);
	if (data_size < 0) {
		gnutls_assert();
		return data_size;
	}

    	ret = _gnutls_send_handshake(cd, state, data, data_size, GNUTLS_CLIENT_KEY_EXCHANGE);
	gnutls_free(data);

	if (ret<0) {
		gnutls_assert();
		return ret;
	}

	return ret;
}

/* Only used in SRP currently
 */
int _gnutls_send_client_kx_message0(SOCKET cd, GNUTLS_STATE state)
{
	uint8 *data;
	int data_size;
	int ret = 0;

	if ( state->gnutls_internals.auth_struct->gnutls_generate_client_kx0 == NULL)
		return 0;

#ifdef HANDSHAKE_DEBUG
	fprintf(stderr, "Sending client KX message0\n");
#endif

	data_size = state->gnutls_internals.auth_struct->gnutls_generate_client_kx0( state->gnutls_key, &data);
	if (data_size < 0) {
		gnutls_assert();
		return data_size;
	}

	ret = _gnutls_send_handshake(cd, state, data, data_size, GNUTLS_CLIENT_KEY_EXCHANGE);
	gnutls_free(data);

	return ret;
}


/* This is the function for the client to send the certificate
 * verify message
 * FIXME: this function does almost nothing except sending garbage to
 * peer.
 */
int _gnutls_send_client_certificate_verify(SOCKET cd, GNUTLS_STATE state)
{
	uint8 *data;
	int ret = 0;
	int data_size;

	/* if certificate verify is not needed just exit */
	if (state->gnutls_internals.certificate_verify_needed==0) return 0;

	if (state->gnutls_internals.auth_struct->gnutls_generate_client_cert_vrfy==NULL) {
		return 0; /* this algorithm does not support cli_cert_vrfy */
	}
	
#ifdef HANDSHAKE_DEBUG
	fprintf(stderr, "Sending client certificate verify message\n");
#endif
	data_size = state->gnutls_internals.auth_struct->gnutls_generate_client_cert_vrfy( state->gnutls_key, &data);
	if (data_size < 0) 
		return data_size;
	ret =
	    _gnutls_send_handshake(cd, state, data,
				   data_size,
				   GNUTLS_CERTIFICATE_VERIFY);
	gnutls_free(data);

	return ret;
}


int _gnutls_recv_server_kx_message(SOCKET cd, GNUTLS_STATE state)
{
	uint8 *data;
	int datasize;
	int ret = 0;

	if (state->gnutls_internals.auth_struct->gnutls_process_server_kx!=NULL) {

#ifdef HANDSHAKE_DEBUG
		fprintf(stderr, "Receiving Server KX message\n");
#endif

		ret =
		    _gnutls_recv_handshake(cd, state, &data,
				   &datasize,
				   GNUTLS_SERVER_KEY_EXCHANGE);
		if (ret < 0)
			return ret;


		ret = state->gnutls_internals.auth_struct->gnutls_process_server_kx( state->gnutls_key, data, datasize);
		gnutls_free(data);
		if (ret < 0)
			return ret;
		
	}
	return ret;
}

int _gnutls_recv_server_kx_message2(SOCKET cd, GNUTLS_STATE state)
{
	uint8 *data;
	int datasize;
	int ret = 0;


	if (state->gnutls_internals.auth_struct->gnutls_process_server_kx2 != NULL) {

#ifdef HANDSHAKE_DEBUG
		fprintf(stderr, "Receiving Server KX message2\n");
#endif

		ret =
		    _gnutls_recv_handshake(cd, state, &data,
				   &datasize,
				   GNUTLS_SERVER_KEY_EXCHANGE);
		if (ret < 0)
			return ret;


		ret = state->gnutls_internals.auth_struct->gnutls_process_server_kx2( state->gnutls_key, data, datasize);
		gnutls_free(data);
		if (ret < 0)
			return ret;

	}
	return ret;
}

int _gnutls_recv_client_kx_message(SOCKET cd, GNUTLS_STATE state)
{
	uint8 *data;
	int datasize;
	int ret = 0;


	/* Do key exchange only if the algorithm permits it */
	if (state->gnutls_internals.auth_struct->gnutls_process_client_kx != NULL) {

#ifdef HANDSHAKE_DEBUG
		fprintf(stderr, "Receiving client KX message\n");
#endif

		ret =
		    _gnutls_recv_handshake(cd, state, &data,
					   &datasize,
					   GNUTLS_CLIENT_KEY_EXCHANGE);
		if (ret < 0)
			return ret;

		ret = state->gnutls_internals.auth_struct->gnutls_process_client_kx( state->gnutls_key, data, datasize);
		gnutls_free(data);
		if (ret < 0)
			return ret;

	}

	return ret;
}

/* only used in SRP */
int _gnutls_recv_client_kx_message0(SOCKET cd, GNUTLS_STATE state)
{
	uint8 *data;
	int datasize;
	int ret = 0;

	/* Do key exchange only if the algorithm permits it */
	if (state->gnutls_internals.auth_struct->gnutls_process_client_kx0 != NULL) {

#ifdef HANDSHAKE_DEBUG
		fprintf(stderr, "Receiving client KX message0\n");
#endif

		ret =
		    _gnutls_recv_handshake(cd, state, &data,
					   &datasize,
					   GNUTLS_CLIENT_KEY_EXCHANGE);
		if (ret < 0)
			return ret;

		ret = state->gnutls_internals.auth_struct->gnutls_process_client_kx0( state->gnutls_key, data, datasize);
		gnutls_free(data);
		if (ret < 0)
			return ret;

	}
	return ret;
}

/* This is called when we want send our certificate
 */
int _gnutls_send_certificate(SOCKET cd, GNUTLS_STATE state)
{
	uint8 *data = NULL;
	int data_size = 0;
	int ret = 0;


	if (state->gnutls_internals.auth_struct->gnutls_generate_certificate==NULL) 
		return 0;

#ifdef HANDSHAKE_DEBUG
	fprintf(stderr, "Sending certificate message\n");
#endif


	data_size = state->gnutls_internals.auth_struct->gnutls_generate_certificate( state->gnutls_key, &data);

	if (data_size < 0) {
		gnutls_assert();
		return data_size;
	}

	ret = _gnutls_send_handshake(cd, state, data, data_size, GNUTLS_CERTIFICATE);
	gnutls_free(data);
	
	if (ret<0) {
		gnutls_assert();
		return ret;
	}

	return data_size;
}

int _gnutls_recv_certificate(SOCKET cd, GNUTLS_STATE state)
{
	int datasize;
	opaque * data;
	int ret = 0;

	if (state->gnutls_internals.auth_struct->gnutls_process_certificate!=NULL) {

		ret =
		    _gnutls_recv_handshake(cd, state, &data,
					   &datasize,
					   GNUTLS_CERTIFICATE);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}

		ret = state->gnutls_internals.auth_struct->gnutls_process_certificate( state->gnutls_key, data, datasize);
		gnutls_free(data);
		if (ret < 0) {
			gnutls_assert();
			return ret;
		}
	}

	return ret;
}
