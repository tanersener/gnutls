/*
 *      Copyright (C) 2001,2002 Nikos Mavroyanopoulos
 *
 * This file is part of GNUTLS.
 *
 * GNUTLS-EXTRA is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * GNUTLS-EXTRA is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include "gnutls_int.h"

#ifdef ENABLE_SRP

#include "gnutls_auth_int.h"
#include "auth_srp.h"
#include "gnutls_errors.h"
#include "gnutls_algorithms.h"

int _gnutls_srp_recv_params( gnutls_session state, const opaque* data, int data_size) {
	uint8 len;

	if (_gnutls_kx_priority( state, GNUTLS_KX_SRP) < 0) {
		/* algorithm was not allowed in this state
		 */
		return 0;
	}
	
	if (state->security_parameters.entity == GNUTLS_SERVER) {
		if (data_size > 0) {
			len = data[0];
			if (len > data_size) {
				gnutls_assert();
				return GNUTLS_E_UNEXPECTED_PACKET_LENGTH;
			}
			if ( sizeof( state->security_parameters.extensions.srp_username) <= len) {
				gnutls_assert();
				return GNUTLS_E_MEMORY_ERROR;
			}
			memcpy( state->security_parameters.extensions.srp_username, &data[1], len);
			state->security_parameters.extensions.srp_username[len]=0; /* null terminated */
		}
	} else { /* client side reading server hello extensions */
		if (state->internals.resumed==RESUME_FALSE)
			return proc_srp_server_hello( state, data, data_size);
		else /* we do not need to process this if
		      * we are resuming.
		      */
			return 0;
	}
	return 0;
}

/* returns data_size or a negative number on failure
 * data is allocated localy
 */
int _gnutls_srp_send_params( gnutls_session state, opaque* data, int data_size) {
	uint8 len;

	if (_gnutls_kx_priority( state, GNUTLS_KX_SRP) < 0) {
		/* algorithm was not allowed in this state
		 */
		return 0;
	}

	/* this function sends the client extension data (username) */
	if (state->security_parameters.entity == GNUTLS_CLIENT) {
		const gnutls_srp_client_credentials cred = _gnutls_get_cred( state->gnutls_key, GNUTLS_CRD_SRP, NULL);

		if (cred==NULL) return 0;

		if (cred->username!=NULL) { /* send username */
			len = strlen(cred->username);
			if (data_size < len+1) {
				gnutls_assert();
				return GNUTLS_E_INVALID_REQUEST;
			}

			data[0] = len;
			memcpy( &data[1], cred->username, len);
			return len + 1;
		}
	} else { /* SERVER SIDE sending (g,n,s) */
		/* We only send the packet if we are NOT
		 * resuming AND we are using SRP
		 */
		
		/* note that security parameters are not fully established
		 */
		if ( _gnutls_cipher_suite_get_kx_algo(state->security_parameters.current_cipher_suite) != GNUTLS_KX_SRP)
			return 0; /* no data to send */
		
		if (state->internals.resumed==RESUME_FALSE)
			return gen_srp_server_hello( state, data, data_size);
		else
			return 0;
	}
	return 0;
}

#endif /* ENABLE_SRP */
