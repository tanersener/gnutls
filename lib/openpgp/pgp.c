/*
 * Copyright (C) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010
 * Free Software Foundation, Inc.
 *
 * Author: Timo Schulz, Nikos Mavrogiannopoulos
 *
 * This file is part of GnuTLS.
 *
 * The GnuTLS is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 *
 */

/* Functions on OpenPGP key parsing
 */

#include <gnutls_int.h>
#include <gnutls_datum.h>
#include <gnutls_global.h>
#include <gnutls_errors.h>
#include <openpgp_int.h>
#include <gnutls_str.h>
#include <gnutls_num.h>
#include <x509/common.h>

/**
 * gnutls_openpgp_crt_init:
 * @key: The structure to be initialized
 *
 * This function will initialize an OpenPGP key structure.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, or an error code.
 **/
int
gnutls_openpgp_crt_init (gnutls_openpgp_crt_t * key)
{
  *key = gnutls_calloc (1, sizeof (gnutls_openpgp_crt_int));

  if (*key)
    return 0;			/* success */
  return GNUTLS_E_MEMORY_ERROR;
}

/**
 * gnutls_openpgp_crt_deinit:
 * @key: The structure to be initialized
 *
 * This function will deinitialize a key structure.
 **/
void
gnutls_openpgp_crt_deinit (gnutls_openpgp_crt_t key)
{
  if (!key)
    return;

  if (key->knode)
    {
      cdk_kbnode_release (key->knode);
      key->knode = NULL;
    }

  gnutls_free (key);
}

/**
 * gnutls_openpgp_crt_import:
 * @key: The structure to store the parsed key.
 * @data: The RAW or BASE64 encoded key.
 * @format: One of gnutls_openpgp_crt_fmt_t elements.
 *
 * This function will convert the given RAW or Base64 encoded key to
 * the native #gnutls_openpgp_crt_t format. The output will be stored
 * in 'key'.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, or an error code.
 **/
int
gnutls_openpgp_crt_import (gnutls_openpgp_crt_t key,
			   const gnutls_datum_t * data,
			   gnutls_openpgp_crt_fmt_t format)
{
  cdk_stream_t inp;
  cdk_packet_t pkt;
  int rc;

  if (data->data == NULL || data->size == 0)
    {
      gnutls_assert ();
      return GNUTLS_E_OPENPGP_GETKEY_FAILED;
    }

  if (format == GNUTLS_OPENPGP_FMT_RAW)
    {
      rc = cdk_kbnode_read_from_mem (&key->knode, data->data, data->size);
      if (rc)
	{
	  rc = _gnutls_map_cdk_rc (rc);
	  gnutls_assert ();
	  return rc;
	}
    }
  else
    {
      rc = cdk_stream_tmp_from_mem (data->data, data->size, &inp);
      if (rc)
	{
	  rc = _gnutls_map_cdk_rc (rc);
	  gnutls_assert ();
	  return rc;
	}
      if (cdk_armor_filter_use (inp))
	rc = cdk_stream_set_armor_flag (inp, 0);
      if (!rc)
	rc = cdk_keydb_get_keyblock (inp, &key->knode);
      cdk_stream_close (inp);
      if (rc)
	{
	  if (rc == CDK_Inv_Packet)
	    rc = GNUTLS_E_OPENPGP_GETKEY_FAILED;
	  else
	    rc = _gnutls_map_cdk_rc (rc);
	  gnutls_assert ();
	  return rc;
	}
    }

  /* Test if the import was successful. */
  pkt = cdk_kbnode_find_packet (key->knode, CDK_PKT_PUBLIC_KEY);
  if (pkt == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_OPENPGP_GETKEY_FAILED;
    }

  return 0;
}

/* internal version of export
 */
int
_gnutls_openpgp_export (cdk_kbnode_t node,
			gnutls_openpgp_crt_fmt_t format,
			void *output_data,
			size_t * output_data_size, int private)
{
  size_t input_data_size = *output_data_size;
  size_t calc_size;
  int rc;

  rc = cdk_kbnode_write_to_mem (node, output_data, output_data_size);
  if (rc)
    {
      rc = _gnutls_map_cdk_rc (rc);
      gnutls_assert ();
      return rc;
    }

  /* If the caller uses output_data == NULL then return what he expects.
   */
  if (!output_data)
    {
      gnutls_assert ();
      return GNUTLS_E_SHORT_MEMORY_BUFFER;
    }

  if (format == GNUTLS_OPENPGP_FMT_BASE64)
    {
      unsigned char *in = gnutls_calloc (1, *output_data_size);
      memcpy (in, output_data, *output_data_size);

      /* Calculate the size of the encoded data and check if the provided
         buffer is large enough. */
      rc = cdk_armor_encode_buffer (in, *output_data_size,
				    NULL, 0, &calc_size,
				    private ? CDK_ARMOR_SECKEY :
				    CDK_ARMOR_PUBKEY);
      if (rc || calc_size > input_data_size)
	{
	  gnutls_free (in);
	  *output_data_size = calc_size;
	  gnutls_assert ();
	  return GNUTLS_E_SHORT_MEMORY_BUFFER;
	}

      rc = cdk_armor_encode_buffer (in, *output_data_size,
				    output_data, input_data_size, &calc_size,
				    private ? CDK_ARMOR_SECKEY :
				    CDK_ARMOR_PUBKEY);
      gnutls_free (in);
      *output_data_size = calc_size;

      if (rc)
	{
	  rc = _gnutls_map_cdk_rc (rc);
	  gnutls_assert ();
	  return rc;
	}
    }

  return 0;

}

/**
 * gnutls_openpgp_crt_export:
 * @key: Holds the key.
 * @format: One of gnutls_openpgp_crt_fmt_t elements.
 * @output_data: will contain the key base64 encoded or raw
 * @output_data_size: holds the size of output_data (and will
 *   be replaced by the actual size of parameters)
 *
 * This function will convert the given key to RAW or Base64 format.
 * If the buffer provided is not long enough to hold the output, then
 * %GNUTLS_E_SHORT_MEMORY_BUFFER will be returned.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, or an error code.
 **/
int
gnutls_openpgp_crt_export (gnutls_openpgp_crt_t key,
			   gnutls_openpgp_crt_fmt_t format,
			   void *output_data, size_t * output_data_size)
{
  return _gnutls_openpgp_export (key->knode, format, output_data,
				 output_data_size, 0);
}

/**
 * gnutls_openpgp_crt_get_fingerprint:
 * @key: the raw data that contains the OpenPGP public key.
 * @fpr: the buffer to save the fingerprint, must hold at least 20 bytes.
 * @fprlen: the integer to save the length of the fingerprint.
 *
 * Get key fingerprint.  Depending on the algorithm, the fingerprint
 * can be 16 or 20 bytes.
 *
 * Returns: On success, 0 is returned.  Otherwise, an error code.
 **/
int
gnutls_openpgp_crt_get_fingerprint (gnutls_openpgp_crt_t key,
				    void *fpr, size_t * fprlen)
{
  cdk_packet_t pkt;
  cdk_pkt_pubkey_t pk = NULL;

  if (!fpr || !fprlen)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  *fprlen = 0;

  pkt = cdk_kbnode_find_packet (key->knode, CDK_PKT_PUBLIC_KEY);
  if (!pkt)
    return GNUTLS_E_OPENPGP_GETKEY_FAILED;

  pk = pkt->pkt.public_key;
  *fprlen = 20;

  /* FIXME: Check if the draft allows old PGP keys. */
  if (is_RSA (pk->pubkey_algo) && pk->version < 4)
    *fprlen = 16;
  cdk_pk_get_fingerprint (pk, fpr);

  return 0;
}

static int
_gnutls_openpgp_count_key_names (gnutls_openpgp_crt_t key)
{
  cdk_kbnode_t p, ctx;
  cdk_packet_t pkt;
  int nuids;

  if (key == NULL)
    {
      gnutls_assert ();
      return 0;
    }

  ctx = NULL;
  nuids = 0;
  while ((p = cdk_kbnode_walk (key->knode, &ctx, 0)))
    {
      pkt = cdk_kbnode_get_packet (p);
      if (pkt->pkttype == CDK_PKT_USER_ID)
	nuids++;
    }

  return nuids;
}


/**
 * gnutls_openpgp_crt_get_name:
 * @key: the structure that contains the OpenPGP public key.
 * @idx: the index of the ID to extract
 * @buf: a pointer to a structure to hold the name, may be %NULL
 *       to only get the @sizeof_buf.
 * @sizeof_buf: holds the maximum size of @buf, on return hold the
 *   actual/required size of @buf.
 *
 * Extracts the userID from the parsed OpenPGP key.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, and if the index of the ID
 *   does not exist %GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE, or an
 *   error code.
 **/
int
gnutls_openpgp_crt_get_name (gnutls_openpgp_crt_t key,
			     int idx, char *buf, size_t * sizeof_buf)
{
  cdk_kbnode_t ctx = NULL, p;
  cdk_packet_t pkt = NULL;
  cdk_pkt_userid_t uid = NULL;
  int pos = 0;

  if (!key)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  if (idx < 0 || idx >= _gnutls_openpgp_count_key_names (key))
    return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;

  pos = 0;
  while ((p = cdk_kbnode_walk (key->knode, &ctx, 0)))
    {
      pkt = cdk_kbnode_get_packet (p);
      if (pkt->pkttype == CDK_PKT_USER_ID)
	{
	  if (pos == idx)
	    break;
	  pos++;
	}
    }

  if (!pkt)
    {
      gnutls_assert ();
      return GNUTLS_E_INTERNAL_ERROR;
    }

  uid = pkt->pkt.user_id;
  if (uid->len >= *sizeof_buf)
    {
      gnutls_assert ();
      *sizeof_buf = uid->len + 1;
      return GNUTLS_E_SHORT_MEMORY_BUFFER;
    }

  if (buf)
    {
      memcpy (buf, uid->name, uid->len);
      buf[uid->len] = '\0';	/* make sure it's a string */
    }
  *sizeof_buf = uid->len + 1;

  if (uid->is_revoked)
    return GNUTLS_E_OPENPGP_UID_REVOKED;

  return 0;
}

/**
 * gnutls_openpgp_crt_get_pk_algorithm:
 * @key: is an OpenPGP key
 * @bits: if bits is non null it will hold the size of the parameters' in bits
 *
 * This function will return the public key algorithm of an OpenPGP
 * certificate.
 *
 * If bits is non null, it should have enough size to hold the parameters
 * size in bits. For RSA the bits returned is the modulus.
 * For DSA the bits returned are of the public exponent.
 *
 * Returns: a member of the #gnutls_pk_algorithm_t enumeration on
 *   success, or a negative value on error.
 **/
gnutls_pk_algorithm_t
gnutls_openpgp_crt_get_pk_algorithm (gnutls_openpgp_crt_t key,
				     unsigned int *bits)
{
  cdk_packet_t pkt;
  int algo;

  if (!key)
    {
      gnutls_assert ();
      return GNUTLS_PK_UNKNOWN;
    }

  algo = 0;
  pkt = cdk_kbnode_find_packet (key->knode, CDK_PKT_PUBLIC_KEY);
  if (pkt)
    {
      if (bits)
	*bits = cdk_pk_get_nbits (pkt->pkt.public_key);
      algo = _gnutls_openpgp_get_algo (pkt->pkt.public_key->pubkey_algo);
    }

  return algo;
}


/**
 * gnutls_openpgp_crt_get_version:
 * @key: the structure that contains the OpenPGP public key.
 *
 * Extract the version of the OpenPGP key.
 *
 * Returns: the version number is returned, or a negative value on errors.
 **/
int
gnutls_openpgp_crt_get_version (gnutls_openpgp_crt_t key)
{
  cdk_packet_t pkt;
  int version;

  if (!key)
    return -1;

  pkt = cdk_kbnode_find_packet (key->knode, CDK_PKT_PUBLIC_KEY);
  if (pkt)
    version = pkt->pkt.public_key->version;
  else
    version = 0;

  return version;
}


/**
 * gnutls_openpgp_crt_get_creation_time:
 * @key: the structure that contains the OpenPGP public key.
 *
 * Get key creation time.
 *
 * Returns: the timestamp when the OpenPGP key was created.
 **/
time_t
gnutls_openpgp_crt_get_creation_time (gnutls_openpgp_crt_t key)
{
  cdk_packet_t pkt;
  time_t timestamp;

  if (!key)
    return (time_t) - 1;

  pkt = cdk_kbnode_find_packet (key->knode, CDK_PKT_PUBLIC_KEY);
  if (pkt)
    timestamp = pkt->pkt.public_key->timestamp;
  else
    timestamp = 0;

  return timestamp;
}


/**
 * gnutls_openpgp_crt_get_expiration_time:
 * @key: the structure that contains the OpenPGP public key.
 *
 * Get key expiration time.  A value of '0' means that the key doesn't
 * expire at all.
 *
 * Returns: the time when the OpenPGP key expires.
 **/
time_t
gnutls_openpgp_crt_get_expiration_time (gnutls_openpgp_crt_t key)
{
  cdk_packet_t pkt;
  time_t expiredate;

  if (!key)
    return (time_t) - 1;

  pkt = cdk_kbnode_find_packet (key->knode, CDK_PKT_PUBLIC_KEY);
  if (pkt)
    expiredate = pkt->pkt.public_key->expiredate;
  else
    expiredate = 0;

  return expiredate;
}

/**
 * gnutls_openpgp_crt_get_key_id:
 * @key: the structure that contains the OpenPGP public key.
 * @keyid: the buffer to save the keyid.
 *
 * Get key id string.
 *
 * Returns: the 64-bit keyID of the OpenPGP key.
 *
 * Since: 2.4.0
 **/
int
gnutls_openpgp_crt_get_key_id (gnutls_openpgp_crt_t key,
			       gnutls_openpgp_keyid_t keyid)
{
  cdk_packet_t pkt;
  uint32_t kid[2];

  if (!key || !keyid)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  pkt = cdk_kbnode_find_packet (key->knode, CDK_PKT_PUBLIC_KEY);
  if (!pkt)
    return GNUTLS_E_OPENPGP_GETKEY_FAILED;

  cdk_pk_get_keyid (pkt->pkt.public_key, kid);
  _gnutls_write_uint32 (kid[0], keyid);
  _gnutls_write_uint32 (kid[1], keyid + 4);

  return 0;
}

/**
 * gnutls_openpgp_crt_get_revoked_status:
 * @key: the structure that contains the OpenPGP public key.
 *
 * Get revocation status of key.
 *
 * Returns: true (1) if the key has been revoked, or false (0) if it
 *   has not.
 *
 * Since: 2.4.0
 **/
int
gnutls_openpgp_crt_get_revoked_status (gnutls_openpgp_crt_t key)
{
  cdk_packet_t pkt;

  if (!key)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  pkt = cdk_kbnode_find_packet (key->knode, CDK_PKT_PUBLIC_KEY);
  if (!pkt)
    return GNUTLS_E_OPENPGP_GETKEY_FAILED;

  if (pkt->pkt.public_key->is_revoked != 0)
    return 1;
  return 0;
}

/**
 * gnutls_openpgp_crt_check_hostname:
 * @key: should contain a #gnutls_openpgp_crt_t structure
 * @hostname: A null terminated string that contains a DNS name
 *
 * This function will check if the given key's owner matches the
 * given hostname. This is a basic implementation of the matching
 * described in RFC2818 (HTTPS), which takes into account wildcards.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, or an error code.
 **/
int
gnutls_openpgp_crt_check_hostname (gnutls_openpgp_crt_t key,
				   const char *hostname)
{
  char dnsname[MAX_CN];
  size_t dnsnamesize;
  int ret = 0;
  int i;

  /* Check through all included names. */
  for (i = 0; !(ret < 0); i++)
    {
      dnsnamesize = sizeof (dnsname);
      ret = gnutls_openpgp_crt_get_name (key, i, dnsname, &dnsnamesize);

      if (ret == 0)
	{
	  /* Length returned by gnutls_openpgp_crt_get_name includes
	     the terminating zero. */
	  dnsnamesize--;

	  if (_gnutls_hostname_compare (dnsname, dnsnamesize, hostname))
	    return 1;
	}
    }

  /* not found a matching name */
  return 0;
}

unsigned int
_gnutls_get_pgp_key_usage (unsigned int cdk_usage)
{
  unsigned int usage = 0;

  if (cdk_usage & CDK_KEY_USG_CERT_SIGN)
    usage |= GNUTLS_KEY_KEY_CERT_SIGN;
  if (cdk_usage & CDK_KEY_USG_DATA_SIGN)
    usage |= GNUTLS_KEY_DIGITAL_SIGNATURE;
  if (cdk_usage & CDK_KEY_USG_COMM_ENCR)
    usage |= GNUTLS_KEY_KEY_ENCIPHERMENT;
  if (cdk_usage & CDK_KEY_USG_STORAGE_ENCR)
    usage |= GNUTLS_KEY_DATA_ENCIPHERMENT;
  if (cdk_usage & CDK_KEY_USG_AUTH)
    usage |= GNUTLS_KEY_KEY_AGREEMENT;

  return usage;
}

/**
 * gnutls_openpgp_crt_get_key_usage:
 * @key: should contain a gnutls_openpgp_crt_t structure
 * @key_usage: where the key usage bits will be stored
 *
 * This function will return certificate's key usage, by checking the
 * key algorithm. The key usage value will ORed values of the:
 * %GNUTLS_KEY_DIGITAL_SIGNATURE, %GNUTLS_KEY_KEY_ENCIPHERMENT.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, or an error code.
 */
int
gnutls_openpgp_crt_get_key_usage (gnutls_openpgp_crt_t key,
				  unsigned int *key_usage)
{
  cdk_packet_t pkt;

  if (!key)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  pkt = cdk_kbnode_find_packet (key->knode, CDK_PKT_PUBLIC_KEY);
  if (!pkt)
    return GNUTLS_E_OPENPGP_GETKEY_FAILED;

  *key_usage = _gnutls_get_pgp_key_usage (pkt->pkt.public_key->pubkey_usage);

  return 0;
}

/**
 * gnutls_openpgp_crt_get_subkey_count:
 * @key: is an OpenPGP key
 *
 * This function will return the number of subkeys present in the
 * given OpenPGP certificate.
 *
 * Returns: the number of subkeys, or a negative value on error.
 *
 * Since: 2.4.0
 **/
int
gnutls_openpgp_crt_get_subkey_count (gnutls_openpgp_crt_t key)
{
  cdk_kbnode_t p, ctx;
  cdk_packet_t pkt;
  int subkeys;

  if (key == NULL)
    {
      gnutls_assert ();
      return 0;
    }

  ctx = NULL;
  subkeys = 0;
  while ((p = cdk_kbnode_walk (key->knode, &ctx, 0)))
    {
      pkt = cdk_kbnode_get_packet (p);
      if (pkt->pkttype == CDK_PKT_PUBLIC_SUBKEY)
	subkeys++;
    }

  return subkeys;
}

/* returns the subkey with the given index */
static cdk_packet_t
_get_public_subkey (gnutls_openpgp_crt_t key, unsigned int indx)
{
  cdk_kbnode_t p, ctx;
  cdk_packet_t pkt;
  unsigned int subkeys;

  if (key == NULL)
    {
      gnutls_assert ();
      return NULL;
    }

  ctx = NULL;
  subkeys = 0;
  while ((p = cdk_kbnode_walk (key->knode, &ctx, 0)))
    {
      pkt = cdk_kbnode_get_packet (p);
      if (pkt->pkttype == CDK_PKT_PUBLIC_SUBKEY && indx == subkeys++)
	return pkt;
    }

  return NULL;
}

/* returns the key with the given keyid. It can be either key or subkey.
 * depending on what requested:
 *   pkt->pkt.secret_key;
 *   pkt->pkt.public_key;
 */
cdk_packet_t
_gnutls_openpgp_find_key (cdk_kbnode_t knode, uint32_t keyid[2],
			  unsigned int priv)
{
  cdk_kbnode_t p, ctx;
  cdk_packet_t pkt;
  uint32_t local_keyid[2];

  ctx = NULL;
  while ((p = cdk_kbnode_walk (knode, &ctx, 0)))
    {
      pkt = cdk_kbnode_get_packet (p);

      if ((priv == 0
	   && (pkt->pkttype == CDK_PKT_PUBLIC_SUBKEY
	       || pkt->pkttype == CDK_PKT_PUBLIC_KEY)) || (priv != 0
							   && (pkt->pkttype ==
							       CDK_PKT_SECRET_SUBKEY
							       || pkt->pkttype
							       ==
							       CDK_PKT_SECRET_KEY)))
	{
	  if (priv == 0)
	    cdk_pk_get_keyid (pkt->pkt.public_key, local_keyid);
	  else
	    cdk_pk_get_keyid (pkt->pkt.secret_key->pk, local_keyid);

	  if (local_keyid[0] == keyid[0] && local_keyid[1] == keyid[1])
	    {
	      return pkt;
	    }
	}
    }

  gnutls_assert ();
  return NULL;
}

/* returns the key with the given keyid
 * depending on what requested:
 *   pkt->pkt.secret_key;
 *   pkt->pkt.public_key;
 */
int
_gnutls_openpgp_find_subkey_idx (cdk_kbnode_t knode, uint32_t keyid[2],
				 unsigned int priv)
{
  cdk_kbnode_t p, ctx;
  cdk_packet_t pkt;
  int i = 0;
  uint32_t local_keyid[2];

  _gnutls_hard_log ("Looking keyid: %x.%x\n", keyid[0], keyid[1]);

  ctx = NULL;
  while ((p = cdk_kbnode_walk (knode, &ctx, 0)))
    {
      pkt = cdk_kbnode_get_packet (p);

      if ((priv == 0 && (pkt->pkttype == CDK_PKT_PUBLIC_SUBKEY)) ||
	  (priv != 0 && (pkt->pkttype == CDK_PKT_SECRET_SUBKEY)))
	{
	  if (priv == 0)
	    cdk_pk_get_keyid (pkt->pkt.public_key, local_keyid);
	  else
	    cdk_pk_get_keyid (pkt->pkt.secret_key->pk, local_keyid);

	  _gnutls_hard_log ("Found keyid: %x.%x\n", local_keyid[0],
			    local_keyid[1]);
	  if (local_keyid[0] == keyid[0] && local_keyid[1] == keyid[1])
	    {
	      return i;
	    }
	  i++;
	}
    }

  gnutls_assert ();
  return GNUTLS_E_OPENPGP_SUBKEY_ERROR;
}

/**
 * gnutls_openpgp_crt_get_subkey_revoked_status:
 * @key: the structure that contains the OpenPGP public key.
 * @idx: is the subkey index
 *
 * Get subkey revocation status.  A negative value indicates an error.
 *
 * Returns: true (1) if the key has been revoked, or false (0) if it
 *   has not.
 *
 * Since: 2.4.0
 **/
int
gnutls_openpgp_crt_get_subkey_revoked_status (gnutls_openpgp_crt_t key,
					      unsigned int idx)
{
  cdk_packet_t pkt;

  if (!key)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  pkt = _get_public_subkey (key, idx);
  if (!pkt)
    return GNUTLS_E_OPENPGP_GETKEY_FAILED;

  if (pkt->pkt.public_key->is_revoked != 0)
    return 1;
  return 0;
}

/**
 * gnutls_openpgp_crt_get_subkey_pk_algorithm:
 * @key: is an OpenPGP key
 * @idx: is the subkey index
 * @bits: if bits is non null it will hold the size of the parameters' in bits
 *
 * This function will return the public key algorithm of a subkey of an OpenPGP
 * certificate.
 *
 * If bits is non null, it should have enough size to hold the
 * parameters size in bits.  For RSA the bits returned is the modulus.
 * For DSA the bits returned are of the public exponent.
 *
 * Returns: a member of the #gnutls_pk_algorithm_t enumeration on
 *   success, or a negative value on error.
 *
 * Since: 2.4.0
 **/
gnutls_pk_algorithm_t
gnutls_openpgp_crt_get_subkey_pk_algorithm (gnutls_openpgp_crt_t key,
					    unsigned int idx,
					    unsigned int *bits)
{
  cdk_packet_t pkt;
  int algo;

  if (!key)
    {
      gnutls_assert ();
      return GNUTLS_PK_UNKNOWN;
    }

  pkt = _get_public_subkey (key, idx);

  algo = 0;
  if (pkt)
    {
      if (bits)
	*bits = cdk_pk_get_nbits (pkt->pkt.public_key);
      algo = _gnutls_openpgp_get_algo (pkt->pkt.public_key->pubkey_algo);
    }

  return algo;
}

/**
 * gnutls_openpgp_crt_get_subkey_creation_time:
 * @key: the structure that contains the OpenPGP public key.
 * @idx: the subkey index
 *
 * Get subkey creation time.
 *
 * Returns: the timestamp when the OpenPGP sub-key was created.
 *
 * Since: 2.4.0
 **/
time_t
gnutls_openpgp_crt_get_subkey_creation_time (gnutls_openpgp_crt_t key,
					     unsigned int idx)
{
  cdk_packet_t pkt;
  time_t timestamp;

  if (!key)
    return (time_t) - 1;

  pkt = _get_public_subkey (key, idx);
  if (pkt)
    timestamp = pkt->pkt.public_key->timestamp;
  else
    timestamp = 0;

  return timestamp;
}


/**
 * gnutls_openpgp_crt_get_subkey_expiration_time:
 * @key: the structure that contains the OpenPGP public key.
 * @idx: the subkey index
 *
 * Get subkey expiration time.  A value of '0' means that the key
 * doesn't expire at all.
 *
 * Returns: the time when the OpenPGP key expires.
 *
 * Since: 2.4.0
 **/
time_t
gnutls_openpgp_crt_get_subkey_expiration_time (gnutls_openpgp_crt_t key,
					       unsigned int idx)
{
  cdk_packet_t pkt;
  time_t expiredate;

  if (!key)
    return (time_t) - 1;

  pkt = _get_public_subkey (key, idx);
  if (pkt)
    expiredate = pkt->pkt.public_key->expiredate;
  else
    expiredate = 0;

  return expiredate;
}

/**
 * gnutls_openpgp_crt_get_subkey_id:
 * @key: the structure that contains the OpenPGP public key.
 * @idx: the subkey index
 * @keyid: the buffer to save the keyid.
 *
 * Get the subkey's key-id.
 *
 * Returns: the 64-bit keyID of the OpenPGP key.
 **/
int
gnutls_openpgp_crt_get_subkey_id (gnutls_openpgp_crt_t key,
				  unsigned int idx,
				  gnutls_openpgp_keyid_t keyid)
{
  cdk_packet_t pkt;
  uint32_t kid[2];

  if (!key || !keyid)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  pkt = _get_public_subkey (key, idx);
  if (!pkt)
    return GNUTLS_E_OPENPGP_GETKEY_FAILED;

  cdk_pk_get_keyid (pkt->pkt.public_key, kid);
  _gnutls_write_uint32 (kid[0], keyid);
  _gnutls_write_uint32 (kid[1], keyid + 4);

  return 0;
}

/**
 * gnutls_openpgp_crt_get_subkey_fingerprint:
 * @key: the raw data that contains the OpenPGP public key.
 * @idx: the subkey index
 * @fpr: the buffer to save the fingerprint, must hold at least 20 bytes.
 * @fprlen: the integer to save the length of the fingerprint.
 *
 * Get key fingerprint of a subkey.  Depending on the algorithm, the
 * fingerprint can be 16 or 20 bytes.
 *
 * Returns: On success, 0 is returned.  Otherwise, an error code.
 *
 * Since: 2.4.0
 **/
int
gnutls_openpgp_crt_get_subkey_fingerprint (gnutls_openpgp_crt_t key,
					   unsigned int idx,
					   void *fpr, size_t * fprlen)
{
  cdk_packet_t pkt;
  cdk_pkt_pubkey_t pk = NULL;

  if (!fpr || !fprlen)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  *fprlen = 0;

  pkt = _get_public_subkey (key, idx);
  if (!pkt)
    return GNUTLS_E_OPENPGP_GETKEY_FAILED;

  pk = pkt->pkt.public_key;
  *fprlen = 20;

  /* FIXME: Check if the draft allows old PGP keys. */
  if (is_RSA (pk->pubkey_algo) && pk->version < 4)
    *fprlen = 16;
  cdk_pk_get_fingerprint (pk, fpr);

  return 0;
}

/**
 * gnutls_openpgp_crt_get_subkey_idx:
 * @key: the structure that contains the OpenPGP public key.
 * @keyid: the keyid.
 *
 * Get subkey's index.
 *
 * Returns: the index of the subkey or a negative error value.
 *
 * Since: 2.4.0
 **/
int
gnutls_openpgp_crt_get_subkey_idx (gnutls_openpgp_crt_t key,
				   const gnutls_openpgp_keyid_t keyid)
{
  int ret;
  uint32_t kid[2];

  if (!key)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  KEYID_IMPORT (kid, keyid);
  ret = _gnutls_openpgp_find_subkey_idx (key->knode, kid, 0);

  if (ret < 0)
    {
      gnutls_assert ();
    }

  return ret;
}

/**
 * gnutls_openpgp_crt_get_subkey_usage:
 * @key: should contain a gnutls_openpgp_crt_t structure
 * @idx: the subkey index
 * @key_usage: where the key usage bits will be stored
 *
 * This function will return certificate's key usage, by checking the
 * key algorithm.  The key usage value will ORed values of
 * %GNUTLS_KEY_DIGITAL_SIGNATURE or %GNUTLS_KEY_KEY_ENCIPHERMENT.
 *
 * A negative value may be returned in case of parsing error.
 *
 * Returns: key usage value.
 *
 * Since: 2.4.0
 */
int
gnutls_openpgp_crt_get_subkey_usage (gnutls_openpgp_crt_t key,
				     unsigned int idx,
				     unsigned int *key_usage)
{
  cdk_packet_t pkt;

  if (!key)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  pkt = _get_public_subkey (key, idx);
  if (!pkt)
    return GNUTLS_E_OPENPGP_SUBKEY_ERROR;

  *key_usage = _gnutls_get_pgp_key_usage (pkt->pkt.public_key->pubkey_usage);

  return 0;
}

int
_gnutls_read_pgp_mpi (cdk_packet_t pkt, unsigned int priv, size_t idx,
		      bigint_t * m)
{
  size_t buf_size = 512;
  opaque *buf = gnutls_malloc (buf_size);
  int err;
  unsigned int max_pub_params = 0;

  if (priv != 0)
    max_pub_params = cdk_pk_get_npkey (pkt->pkt.secret_key->pk->pubkey_algo);

  if (buf == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_MEMORY_ERROR;
    }

  /* FIXME: Note that opencdk doesn't like the buf to be NULL.
   */
  if (priv == 0)
    err =
      cdk_pk_get_mpi (pkt->pkt.public_key, idx, buf, buf_size, &buf_size,
		      NULL);
  else
    {
      if (idx < max_pub_params)
	err =
	  cdk_pk_get_mpi (pkt->pkt.secret_key->pk, idx, buf, buf_size,
			  &buf_size, NULL);
      else
	{
	  err =
	    cdk_sk_get_mpi (pkt->pkt.secret_key, idx - max_pub_params, buf,
			    buf_size, &buf_size, NULL);
	}
    }

  if (err == CDK_Too_Short)
    {
      buf = gnutls_realloc_fast (buf, buf_size);
      if (buf == NULL)
	{
	  gnutls_assert ();
	  return GNUTLS_E_MEMORY_ERROR;
	}

      if (priv == 0)
	err =
	  cdk_pk_get_mpi (pkt->pkt.public_key, idx, buf, buf_size, &buf_size,
			  NULL);
      else
	{
	  if (idx < max_pub_params)
	    err =
	      cdk_pk_get_mpi (pkt->pkt.secret_key->pk, idx, buf, buf_size,
			      &buf_size, NULL);
	  else
	    {
	      err =
		cdk_sk_get_mpi (pkt->pkt.secret_key, idx - max_pub_params,
				buf, buf_size, &buf_size, NULL);
	    }
	}
    }

  if (err != CDK_Success)
    {
      gnutls_assert ();
      gnutls_free (buf);
      return _gnutls_map_cdk_rc (err);
    }

  err = _gnutls_mpi_scan (m, buf, buf_size);
  gnutls_free (buf);

  if (err < 0)
    {
      gnutls_assert ();
      return err;
    }

  return 0;
}


/* Extracts DSA and RSA parameters from a certificate.
 */
int
_gnutls_openpgp_crt_get_mpis (gnutls_openpgp_crt_t cert,
			      uint32_t * keyid /* [2] */ ,
			      bigint_t * params, int *params_size)
{
  int result, i;
  int pk_algorithm, local_params;
  cdk_packet_t pkt;

  if (keyid == NULL)
    pkt = cdk_kbnode_find_packet (cert->knode, CDK_PKT_PUBLIC_KEY);
  else
    pkt = _gnutls_openpgp_find_key (cert->knode, keyid, 0);

  if (pkt == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_OPENPGP_GETKEY_FAILED;
    }

  pk_algorithm = _gnutls_openpgp_get_algo (pkt->pkt.public_key->pubkey_algo);

  switch (pk_algorithm)
    {
    case GNUTLS_PK_RSA:
      local_params = RSA_PUBLIC_PARAMS;
      break;
    case GNUTLS_PK_DSA:
      local_params = DSA_PUBLIC_PARAMS;
      break;
    default:
      gnutls_assert ();
      return GNUTLS_E_UNSUPPORTED_CERTIFICATE_TYPE;
    }

  if (*params_size < local_params)
    {
      gnutls_assert ();
      return GNUTLS_E_INTERNAL_ERROR;
    }

  *params_size = local_params;

  for (i = 0; i < local_params; i++)
    {
      result = _gnutls_read_pgp_mpi (pkt, 0, i, &params[i]);
      if (result < 0)
	{
	  gnutls_assert ();
	  goto error;
	}
    }

  return 0;

error:
  {
    int j;
    for (j = 0; j < i; j++)
      _gnutls_mpi_release (&params[j]);
  }

  return result;
}

/* The internal version of export
 */
static int
_get_pk_rsa_raw (gnutls_openpgp_crt_t crt, gnutls_openpgp_keyid_t keyid,
		 gnutls_datum_t * m, gnutls_datum_t * e)
{
  int pk_algorithm, ret, i;
  cdk_packet_t pkt;
  uint32_t kid32[2];
  bigint_t params[MAX_PUBLIC_PARAMS_SIZE];
  int params_size = MAX_PUBLIC_PARAMS_SIZE;

  if (crt == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  KEYID_IMPORT (kid32, keyid);

  pkt = _gnutls_openpgp_find_key (crt->knode, kid32, 0);
  if (pkt == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_OPENPGP_GETKEY_FAILED;
    }

  pk_algorithm = _gnutls_openpgp_get_algo (pkt->pkt.public_key->pubkey_algo);

  if (pk_algorithm != GNUTLS_PK_RSA)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  ret = _gnutls_openpgp_crt_get_mpis (crt, kid32, params, &params_size);
  if (ret < 0)
    {
      gnutls_assert ();
      return ret;
    }

  ret = _gnutls_mpi_dprint (params[0], m);
  if (ret < 0)
    {
      gnutls_assert ();
      goto cleanup;
    }

  ret = _gnutls_mpi_dprint (params[1], e);
  if (ret < 0)
    {
      gnutls_assert ();
      _gnutls_free_datum (m);
      goto cleanup;
    }

  ret = 0;

cleanup:
  for (i = 0; i < params_size; i++)
    {
      _gnutls_mpi_release (&params[i]);
    }
  return ret;
}

static int
_get_pk_dsa_raw (gnutls_openpgp_crt_t crt, gnutls_openpgp_keyid_t keyid,
		 gnutls_datum_t * p, gnutls_datum_t * q,
		 gnutls_datum_t * g, gnutls_datum_t * y)
{
  int pk_algorithm, ret, i;
  cdk_packet_t pkt;
  uint32_t kid32[2];
  bigint_t params[MAX_PUBLIC_PARAMS_SIZE];
  int params_size = MAX_PUBLIC_PARAMS_SIZE;

  if (crt == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  KEYID_IMPORT (kid32, keyid);

  pkt = _gnutls_openpgp_find_key (crt->knode, kid32, 0);
  if (pkt == NULL)
    {
      gnutls_assert ();
      return GNUTLS_E_OPENPGP_GETKEY_FAILED;
    }

  pk_algorithm = _gnutls_openpgp_get_algo (pkt->pkt.public_key->pubkey_algo);

  if (pk_algorithm != GNUTLS_PK_DSA)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  ret = _gnutls_openpgp_crt_get_mpis (crt, kid32, params, &params_size);
  if (ret < 0)
    {
      gnutls_assert ();
      return ret;
    }

  /* P */
  ret = _gnutls_mpi_dprint (params[0], p);
  if (ret < 0)
    {
      gnutls_assert ();
      goto cleanup;
    }

  /* Q */
  ret = _gnutls_mpi_dprint (params[1], q);
  if (ret < 0)
    {
      gnutls_assert ();
      _gnutls_free_datum (p);
      goto cleanup;
    }


  /* G */
  ret = _gnutls_mpi_dprint (params[2], g);
  if (ret < 0)
    {
      gnutls_assert ();
      _gnutls_free_datum (p);
      _gnutls_free_datum (q);
      goto cleanup;
    }


  /* Y */
  ret = _gnutls_mpi_dprint (params[3], y);
  if (ret < 0)
    {
      gnutls_assert ();
      _gnutls_free_datum (p);
      _gnutls_free_datum (g);
      _gnutls_free_datum (q);
      goto cleanup;
    }

  ret = 0;

cleanup:
  for (i = 0; i < params_size; i++)
    {
      _gnutls_mpi_release (&params[i]);
    }
  return ret;
}


/**
 * gnutls_openpgp_crt_get_pk_rsa_raw:
 * @crt: Holds the certificate
 * @m: will hold the modulus
 * @e: will hold the public exponent
 *
 * This function will export the RSA public key's parameters found in
 * the given structure.  The new parameters will be allocated using
 * gnutls_malloc() and will be stored in the appropriate datum.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise an error.
 *
 * Since: 2.4.0
 **/
int
gnutls_openpgp_crt_get_pk_rsa_raw (gnutls_openpgp_crt_t crt,
				   gnutls_datum_t * m, gnutls_datum_t * e)
{
  gnutls_openpgp_keyid_t keyid;
  int ret;

  ret = gnutls_openpgp_crt_get_key_id (crt, keyid);
  if (ret < 0)
    {
      gnutls_assert ();
      return ret;
    }

  return _get_pk_rsa_raw (crt, keyid, m, e);
}

/**
 * gnutls_openpgp_crt_get_pk_dsa_raw:
 * @crt: Holds the certificate
 * @p: will hold the p
 * @q: will hold the q
 * @g: will hold the g
 * @y: will hold the y
 *
 * This function will export the DSA public key's parameters found in
 * the given certificate.  The new parameters will be allocated using
 * gnutls_malloc() and will be stored in the appropriate datum.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise an error.
 *
 * Since: 2.4.0
 **/
int
gnutls_openpgp_crt_get_pk_dsa_raw (gnutls_openpgp_crt_t crt,
				   gnutls_datum_t * p, gnutls_datum_t * q,
				   gnutls_datum_t * g, gnutls_datum_t * y)
{
  gnutls_openpgp_keyid_t keyid;
  int ret;

  ret = gnutls_openpgp_crt_get_key_id (crt, keyid);
  if (ret < 0)
    {
      gnutls_assert ();
      return ret;
    }

  return _get_pk_dsa_raw (crt, keyid, p, q, g, y);
}

/**
 * gnutls_openpgp_crt_get_subkey_pk_rsa_raw:
 * @crt: Holds the certificate
 * @idx: Is the subkey index
 * @m: will hold the modulus
 * @e: will hold the public exponent
 *
 * This function will export the RSA public key's parameters found in
 * the given structure.  The new parameters will be allocated using
 * gnutls_malloc() and will be stored in the appropriate datum.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise an error.
 *
 * Since: 2.4.0
 **/
int
gnutls_openpgp_crt_get_subkey_pk_rsa_raw (gnutls_openpgp_crt_t crt,
					  unsigned int idx,
					  gnutls_datum_t * m,
					  gnutls_datum_t * e)
{
  gnutls_openpgp_keyid_t keyid;
  int ret;

  ret = gnutls_openpgp_crt_get_subkey_id (crt, idx, keyid);
  if (ret < 0)
    {
      gnutls_assert ();
      return ret;
    }

  return _get_pk_rsa_raw (crt, keyid, m, e);
}

/**
 * gnutls_openpgp_crt_get_subkey_pk_dsa_raw:
 * @crt: Holds the certificate
 * @idx: Is the subkey index
 * @p: will hold the p
 * @q: will hold the q
 * @g: will hold the g
 * @y: will hold the y
 *
 * This function will export the DSA public key's parameters found in
 * the given certificate.  The new parameters will be allocated using
 * gnutls_malloc() and will be stored in the appropriate datum.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, otherwise an error.
 *
 * Since: 2.4.0
 **/
int
gnutls_openpgp_crt_get_subkey_pk_dsa_raw (gnutls_openpgp_crt_t crt,
					  unsigned int idx,
					  gnutls_datum_t * p,
					  gnutls_datum_t * q,
					  gnutls_datum_t * g,
					  gnutls_datum_t * y)
{
  gnutls_openpgp_keyid_t keyid;
  int ret;

  ret = gnutls_openpgp_crt_get_subkey_id (crt, idx, keyid);
  if (ret < 0)
    {
      gnutls_assert ();
      return ret;
    }

  return _get_pk_dsa_raw (crt, keyid, p, q, g, y);
}

/**
 * gnutls_openpgp_crt_get_preferred_key_id:
 * @key: the structure that contains the OpenPGP public key.
 * @keyid: the struct to save the keyid.
 *
 * Get preferred key id.  If it hasn't been set it returns
 * %GNUTLS_E_INVALID_REQUEST.
 *
 * Returns: the 64-bit preferred keyID of the OpenPGP key.
 **/
int
gnutls_openpgp_crt_get_preferred_key_id (gnutls_openpgp_crt_t key,
					 gnutls_openpgp_keyid_t keyid)
{
  if (!key || !keyid || !key->preferred_set)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  memcpy (keyid, key->preferred_keyid, sizeof (gnutls_openpgp_keyid_t));

  return 0;
}

/**
 * gnutls_openpgp_crt_set_preferred_key_id:
 * @key: the structure that contains the OpenPGP public key.
 * @keyid: the selected keyid
 *
 * This allows setting a preferred key id for the given certificate.
 * This key will be used by functions that involve key handling.
 *
 * Returns: On success, %GNUTLS_E_SUCCESS (zero) is returned,
 *   otherwise an error code is returned.
 **/
int
gnutls_openpgp_crt_set_preferred_key_id (gnutls_openpgp_crt_t key,
					 const gnutls_openpgp_keyid_t keyid)
{
  int ret;

  if (!key)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  /* check if the id is valid */
  ret = gnutls_openpgp_crt_get_subkey_idx (key, keyid);
  if (ret < 0)
    {
      _gnutls_x509_log ("the requested subkey does not exist\n");
      gnutls_assert ();
      return ret;
    }

  key->preferred_set = 1;
  memcpy (key->preferred_keyid, keyid, sizeof (gnutls_openpgp_keyid_t));

  return 0;
}

/**
 * gnutls_openpgp_crt_get_auth_subkey:
 * @crt: the structure that contains the OpenPGP public key.
 * @keyid: the struct to save the keyid.
 * @flag: Non zero indicates that a valid subkey is always returned.
 *
 * Returns the 64-bit keyID of the first valid OpenPGP subkey marked
 * for authentication.  If flag is non zero and no authentication
 * subkey exists, then a valid subkey will be returned even if it is
 * not marked for authentication.
 * Returns the 64-bit keyID of the first valid OpenPGP subkey marked
 * for authentication.  If flag is non zero and no authentication
 * subkey exists, then a valid subkey will be returned even if it is
 * not marked for authentication.
 *
 * Returns: %GNUTLS_E_SUCCESS on success, or an error code.
 **/
int
gnutls_openpgp_crt_get_auth_subkey (gnutls_openpgp_crt_t crt,
				    gnutls_openpgp_keyid_t keyid,
				    unsigned int flag)
{
  int ret, subkeys, i;
  unsigned int usage;
  unsigned int keyid_init = 0;

  subkeys = gnutls_openpgp_crt_get_subkey_count (crt);
  if (subkeys <= 0)
    {
      gnutls_assert ();
      return GNUTLS_E_OPENPGP_SUBKEY_ERROR;
    }

  /* Try to find a subkey with the authentication flag set.
   * if none exists use the last one found
   */
  for (i = 0; i < subkeys; i++)
    {

      ret = gnutls_openpgp_crt_get_subkey_revoked_status (crt, i);
      if (ret != 0)		/* it is revoked. ignore it */
	continue;

      if (keyid_init == 0)
	{			/* keep the first valid subkey */
	  ret = gnutls_openpgp_crt_get_subkey_id (crt, i, keyid);
	  if (ret < 0)
	    {
	      gnutls_assert ();
	      return ret;
	    }

	  keyid_init = 1;
	}

      ret = gnutls_openpgp_crt_get_subkey_usage (crt, i, &usage);
      if (ret < 0)
	{
	  gnutls_assert ();
	  return ret;
	}

      if (usage & GNUTLS_KEY_KEY_AGREEMENT)
	{
	  ret = gnutls_openpgp_crt_get_subkey_id (crt, i, keyid);
	  if (ret < 0)
	    {
	      gnutls_assert ();
	      return ret;
	    }
	  return 0;
	}
    }

  if (flag && keyid_init)
    return 0;
  else
    return GNUTLS_E_REQUESTED_DATA_NOT_AVAILABLE;
}

/**
 * gnutls_openpgp_crt_verify_hash:
 * @crt: Holds the certificate
 * @flags: should be 0 for now
 * @hash: holds the hash digest to be verified
 * @signature: contains the signature
 *
 * This function will verify the given signed digest, using the
 * parameters from the certificate.
 *
 * Returns: In case of a verification failure 0 is returned, and 1 on
 * success.
 **/
int
gnutls_openpgp_crt_verify_hash (gnutls_openpgp_crt_t crt, unsigned int flags,
			     const gnutls_datum_t * hash,
			     const gnutls_datum_t * signature)
{
  int ret;
  bigint_t params[MAX_PUBLIC_PARAMS_SIZE];
  int params_size = MAX_PUBLIC_PARAMS_SIZE;
  gnutls_pk_algorithm_t pk;
  uint32_t kid[2];
  
  if (crt == NULL || !crt->preferred_set)
    {
      gnutls_assert ();
      return GNUTLS_E_INVALID_REQUEST;
    }

  ret = gnutls_openpgp_crt_get_pk_algorithm (crt, NULL);
  if (ret < 0)
    {
      gnutls_assert ();
      return ret;
    }
  pk = ret;
  
  KEYID_IMPORT (kid, crt->preferred_keyid);
  ret = _gnutls_openpgp_crt_get_mpis (crt, kid, params, &params_size);
  if (ret < 0)
    {
      gnutls_assert ();
      return ret;
    }

  ret = pubkey_verify_sig( NULL, hash, signature, pk, params, params_size);
  if (ret < 0)
    {
      gnutls_assert ();
      return 0;
    }

  return ret;
}
