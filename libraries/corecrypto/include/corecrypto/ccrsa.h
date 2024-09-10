/*
 * Copyright (c) 2010,2011,2012,2014,2015,2016,2017,2018 Apple Inc. All rights reserved.
 *
 * corecrypto Internal Use License Agreement
 *
 * IMPORTANT:  This Apple corecrypto software is supplied to you by Apple Inc. ("Apple")
 * in consideration of your agreement to the following terms, and your download or use
 * of this Apple software constitutes acceptance of these terms.  If you do not agree
 * with these terms, please do not download or use this Apple software.
 *
 * 1.    As used in this Agreement, the term "Apple Software" collectively means and
 * includes all of the Apple corecrypto materials provided by Apple here, including
 * but not limited to the Apple corecrypto software, frameworks, libraries, documentation
 * and other Apple-created materials. In consideration of your agreement to abide by the
 * following terms, conditioned upon your compliance with these terms and subject to
 * these terms, Apple grants you, for a period of ninety (90) days from the date you
 * download the Apple Software, a limited, non-exclusive, non-sublicensable license
 * under Apple’s copyrights in the Apple Software to make a reasonable number of copies
 * of, compile, and run the Apple Software internally within your organization only on
 * devices and computers you own or control, for the sole purpose of verifying the
 * security characteristics and correct functioning of the Apple Software; provided
 * that you must retain this notice and the following text and disclaimers in all
 * copies of the Apple Software that you make. You may not, directly or indirectly,
 * redistribute the Apple Software or any portions thereof. The Apple Software is only
 * licensed and intended for use as expressly stated above and may not be used for other
 * purposes or in other contexts without Apple's prior written permission.  Except as
 * expressly stated in this notice, no other rights or licenses, express or implied, are
 * granted by Apple herein.
 *
 * 2.    The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO
 * WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED WARRANTIES
 * OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, REGARDING
 * THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN COMBINATION WITH YOUR PRODUCTS,
 * SYSTEMS, OR SERVICES. APPLE DOES NOT WARRANT THAT THE APPLE SOFTWARE WILL MEET YOUR
 * REQUIREMENTS, THAT THE OPERATION OF THE APPLE SOFTWARE WILL BE UNINTERRUPTED OR
 * ERROR-FREE, THAT DEFECTS IN THE APPLE SOFTWARE WILL BE CORRECTED, OR THAT THE APPLE
 * SOFTWARE WILL BE COMPATIBLE WITH FUTURE APPLE PRODUCTS, SOFTWARE OR SERVICES. NO ORAL
 * OR WRITTEN INFORMATION OR ADVICE GIVEN BY APPLE OR AN APPLE AUTHORIZED REPRESENTATIVE
 * WILL CREATE A WARRANTY.
 *
 * 3.    IN NO EVENT SHALL APPLE BE LIABLE FOR ANY DIRECT, SPECIAL, INDIRECT, INCIDENTAL
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) ARISING
 * IN ANY WAY OUT OF THE USE, REPRODUCTION, COMPILATION OR OPERATION OF THE APPLE
 * SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT (INCLUDING
 * NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * 4.    This Agreement is effective until terminated. Your rights under this Agreement will
 * terminate automatically without notice from Apple if you fail to comply with any term(s)
 * of this Agreement.  Upon termination, you agree to cease all use of the Apple Software
 * and destroy all copies, full or partial, of the Apple Software. This Agreement will be
 * governed and construed in accordance with the laws of the State of California, without
 * regard to its choice of law rules.
 *
 * You may report security issues about Apple products to product-security@apple.com,
 * as described here:  https://www.apple.com/support/security/.  Non-security bugs and
 * enhancement requests can be made via https://bugreport.apple.com as described
 * here: https://developer.apple.com/bug-reporting/
 *
 * EA1350
 * 10/5/15
 */

#ifndef _CORECRYPTO_CCRSA_H_
#define _CORECRYPTO_CCRSA_H_

#include <corecrypto/cc.h>
#include <corecrypto/ccdigest.h>
#include <corecrypto/ccrng.h>
#include <corecrypto/cczp.h>
#include <stdbool.h>

// Apple does not generate keys of greater than 4096 bits
// This limit is relaxed to accommodate potential third-party consumers
#define CCRSA_KEYGEN_MAX_NBITS 8192

// Program error: buffer too small or encrypted message is too small
#define CCRSA_INVALID_INPUT        -1
// Invalid crypto configuration: Hash length versus RSA key size
#define CCRSA_INVALID_CONFIG       -2
// The data is invalid (we won't say more for security
#define CCRSA_DECRYPTION_ERROR     -3

#define CCRSA_ENCODING_ERROR       -4
#define CCRSA_DECODING_ERROR       -5
#define CCRSA_SIGNATURE_GEN_ERROR  -6

struct ccrsa_full_ctx {
    __CCZP_ELEMENTS_DEFINITIONS(pb_)
} CC_ALIGNED(CCN_UNIT_SIZE);

struct ccrsa_pub_ctx {
    __CCZP_ELEMENTS_DEFINITIONS(pb_)
} CC_ALIGNED(CCN_UNIT_SIZE);

struct ccrsa_priv_ctx {
    __CCZP_ELEMENTS_DEFINITIONS(pv_)
} CC_ALIGNED(CCN_UNIT_SIZE);


#if CORECRYPTO_USE_TRANSPARENT_UNION
    typedef union {
        cczp_t zp;
        struct ccrsa_pub_ctx* pub;
        struct ccrsa_full_ctx *full;
    } ccrsa_full_ctx_t __attribute__((transparent_union));
    typedef struct ccrsa_full_ctx ccrsa_full_ctx;
    typedef struct ccrsa_priv_ctx ccrsa_priv_ctx;

    typedef union {
        cczp_t zp;
        ccrsa_priv_ctx *priv;
    } ccrsa_priv_ctx_t __attribute__((transparent_union));


typedef ccrsa_full_ctx_t ccrsa_pub_ctx_t;
typedef struct ccrsa_pub_ctx ccrsa_pub_ctx;

#else
    typedef struct ccrsa_full_ctx* ccrsa_full_ctx_t;
    typedef struct ccrsa_pub_ctx* ccrsa_pub_ctx_t;
    typedef struct ccrsa_priv_ctx* ccrsa_priv_ctx_t;
#endif



/*
 public key cczp      d=e^-1 mod phi(m) priv key cczp             priv key cczq             dp, dq, qinv
 |                              |       |                         |                         |
 |                              |       |                         |                         |
 +-------+------+-------+------++------++-------+------+---------++-------+------+---------++-------+-------+---------+
 | zm_hd | m[n] |mr[n+1]| e[n] || d[n] || zp_hd |p[n/2]|pr[n/2+1]|| zq_hd |q[n/2]|qr[n/2+1]||dp[n/2]|dq[n/2]|qinv[n/2]|
 +-------+------+-------+------++------++-------+------+---------++-------+------+---------++-------+-------+---------+
 */

 /* Return the size of an ccec_full_ctx where each ccn is _size_ bytes. Get _size_ through ccn_sizeof(nbits) */

/* Return the size of an ccec_full_ctx where each ccn is _size_ bytes. */

#define ccrsa_pub_ctx_size(_size_)   (sizeof(struct cczp) + CCN_UNIT_SIZE + 3 * (_size_))
#define ccrsa_priv_ctx_size(_size_)  ((sizeof(struct cczp) + CCN_UNIT_SIZE) * 2 + 7 * ccn_sizeof(ccn_bitsof_size(_size_)/2 + 1))
#define ccrsa_full_ctx_size(_size_)  (ccrsa_pub_ctx_size(_size_) + _size_ + ccrsa_priv_ctx_size(_size_))

/* Declare a fully scheduled rsa key.  Size is the size in bytes each ccn in
   the key.  For example to declare (on the stack or in a struct) a 1021 bit
   rsa public key named foo use ccrsa_pub_ctx_decl(ccn_sizeof(1021), foo). 
 */
#define ccrsa_full_ctx_decl(_size_, _name_)   cc_ctx_decl(struct ccrsa_full_ctx, ccrsa_full_ctx_size(_size_), _name_)
#define ccrsa_full_ctx_clear(_size_, _name_)  cc_clear(ccrsa_full_ctx_size(_size_), _name_)
#define ccrsa_pub_ctx_decl(_size_, _name_)    cc_ctx_decl(struct ccrsa_pub_ctx, ccrsa_pub_ctx_size(_size_), _name_)
#define ccrsa_pub_ctx_clear(_size_, _name_)   cc_clear(ccrsa_pub_ctx_size(_size_), _name_)

// accessors to ccrsa full and public key fields. */
// The offsets are computed using pb_ccn. If any object other than ccrsa_full_ctx_t
// or ccrsa_pub_ctx_t is passed to the macros, compiler error is generated.



#if CORECRYPTO_USE_TRANSPARENT_UNION
//#define ccrsa_ctx_zm(_ctx_)        (((ccrsa_pub_ctx_t)(_ctx_)).zp)

    CC_CONST CC_INLINE cczp_t ccrsa_ctx_zm(ccrsa_full_ctx_t _ctx_) { return ((cczp_t)(struct cczp *)((_ctx_).full)); }
    CC_CONST CC_INLINE cc_unit  *ccrsa_ctx_m(ccrsa_full_ctx_t _ctx_){ return        ((_ctx_).full->pb_ccn);}
    #define ccrsa_ctx_n(_ctx_)         (ccrsa_ctx_zm(_ctx_).zp->n)
#else
    #define ccrsa_ctx_zm(_ctx_)        ((cczp_t)(_ctx_))
    #define ccrsa_ctx_n(_ctx_)         (ccrsa_ctx_zm(_ctx_)->n)
    #define ccrsa_ctx_m(_ctx_)         ((_ctx_)->pb_ccn)
#endif

#define ccrsa_ctx_e(_ctx_)         (ccrsa_ctx_m(_ctx_) + 2 * ccrsa_ctx_n(_ctx_) + 1)
#define ccrsa_ctx_d(_ctx_)         (ccrsa_ctx_m(_ctx_) + 3 * ccrsa_ctx_n(_ctx_) + 1)

// accessors to ccrsa private key fields
// The offsets are computed using pv_ccn. If any object other than ccrsa_priv_ctx_t
// is passed to the macros, compiler error is generated.
#if CORECRYPTO_USE_TRANSPARENT_UNION

/* rvalue accessors to ccec_key fields. */
CC_CONST CC_INLINE
ccrsa_priv_ctx_t ccrsa_get_private_ctx_ptr(ccrsa_full_ctx_t fk) {
    cc_unit *p = (cc_unit *)fk.full;
    cc_size p_size = ccrsa_ctx_n(fk);
    p += ccn_nof_size(ccrsa_pub_ctx_size(ccn_sizeof_n(p_size))) + p_size;
    ccrsa_priv_ctx *priv = (ccrsa_priv_ctx *)p;
    return (ccrsa_priv_ctx_t)priv;
}

CC_CONST CC_INLINE
ccrsa_pub_ctx_t ccrsa_ctx_public(ccrsa_full_ctx_t fk) {
    return (ccrsa_pub_ctx_t) fk.full;
}

#define ccrsa_ctx_private_zp(FK)   ((ccrsa_get_private_ctx_ptr(FK)).zp)
#define ccrsa_ctx_private_zq(FK)   ((cczp_t)((ccrsa_get_private_ctx_ptr(FK)).zp.zp->ccn + 2 * ccrsa_ctx_private_zp(FK).zp->n + 1))
#define ccrsa_ctx_private_dp(FK)   ((ccrsa_get_private_ctx_ptr(FK)).zp.zp->ccn + 4 * ccrsa_ctx_private_zp(FK).zp->n + 2 + ccn_nof_size(sizeof(struct cczp)))
#define ccrsa_ctx_private_dq(FK)   ((ccrsa_get_private_ctx_ptr(FK)).zp.zp->ccn + 5 * ccrsa_ctx_private_zp(FK).zp->n + 2 + ccn_nof_size(sizeof(struct cczp)))
#define ccrsa_ctx_private_qinv(FK) ((ccrsa_get_private_ctx_ptr(FK)).zp.zp->ccn + 6 * ccrsa_ctx_private_zp(FK).zp->n + 2 + ccn_nof_size(sizeof(struct cczp)))

#else
#define ccrsa_ctx_private_zp(FK)   ((cczp_t)ccrsa_get_private_ctx_ptr(FK))
#define ccrsa_ctx_private_zq(FK)   ((cczp_t)((ccrsa_get_private_ctx_ptr(FK))->pv_ccn + 2 * ccrsa_ctx_private_zp(FK)->n + 1))
#define ccrsa_ctx_private_dp(FK)   ((ccrsa_get_private_ctx_ptr(FK))->pv_ccn + 4 * ccrsa_ctx_private_zp(FK)->n + 2 + ccn_nof_size(sizeof(struct cczp)))
#define ccrsa_ctx_private_dq(FK)   ((ccrsa_get_private_ctx_ptr(FK))->pv_ccn + 5 * ccrsa_ctx_private_zp(FK)->n + 2 + ccn_nof_size(sizeof(struct cczp)))
#define ccrsa_ctx_private_qinv(FK) ((ccrsa_get_private_ctx_ptr(FK))->pv_ccn + 6 * ccrsa_ctx_private_zp(FK)->n + 2 + ccn_nof_size(sizeof(struct cczp)))

CC_CONST CC_INLINE
ccrsa_priv_ctx_t ccrsa_get_private_ctx_ptr(ccrsa_full_ctx_t fk) {
    ccrsa_priv_ctx_t priv = (ccrsa_priv_ctx_t)(ccrsa_ctx_d(fk)+ccrsa_ctx_n(fk));
    return priv;
}

/*!
 @function   ccrsa_ctx_public
 @abstract   gets the public key from full key
 @param      fk      RSA full key
 @result     Returns RSA public ker
 */
CC_CONST CC_INLINE
ccrsa_pub_ctx_t ccrsa_ctx_public(ccrsa_full_ctx_t fk) {
    return (ccrsa_pub_ctx_t) fk;
}

#endif

/* Return exact key bit size */
static inline size_t
ccrsa_pubkeylength(ccrsa_pub_ctx_t pubk) {
    return cczp_bitlen(ccrsa_ctx_zm(pubk));
}

/* PKCS1 pad_markers */
#define CCRSA_PKCS1_PAD_SIGN     1
#define CCRSA_PKCS1_PAD_ENCRYPT  2

/* Initialize key based on modulus and e as cc_unit.  key->zp.n must already be set. */
CC_NONNULL_TU((1)) CC_NONNULL((2, 3))
int ccrsa_init_pub(ccrsa_pub_ctx_t key, const cc_unit *modulus,
                    const cc_unit *e);

/* Initialize key based on modulus and e as big endian byte array
    key->zp.n must already be set. */
CC_NONNULL_TU((1)) CC_NONNULL((3 ,5))
int ccrsa_make_pub(ccrsa_pub_ctx_t pubk,
                              size_t exp_nbytes, const uint8_t *exp,
                              size_t mod_nbytes, const uint8_t *mod);

/* Do a public key crypto operation (typically verify or encrypt) on in and put
   the result in out. Both in and out should be cc_unit aligned and
   ccrsa_key_n(key) units long. Clients should use ccn_read_uint() to
   convert bytes to a cc_unit to use for this API.*/
CC_NONNULL_TU((1)) CC_NONNULL((2, 3))
int ccrsa_pub_crypt(ccrsa_pub_ctx_t key, cc_unit *out, const cc_unit *in);

/* Generate an nbit rsa key pair in key, which should be allocated using
   ccrsa_full_ctx_decl(ccn_sizeof(1024), rsa_ctx). The unsigned big endian
   byte array exponent e of length e_size is used as the exponent. It's an
   error to call this function with an exponent larger than nbits. rng
   must be a pointer to an initialized struct ccrng_state. */
CC_NONNULL_TU((2)) CC_NONNULL((4, 5))
int ccrsa_generate_key(size_t nbits, ccrsa_full_ctx_t rsa_ctx,
                       size_t e_size, const void *e, struct ccrng_state *rng) CC_WARN_RESULT;

/* Generate RSA key in conformance with FIPS186-4 standard */
CC_NONNULL_TU((2)) CC_NONNULL((4, 5, 6))
int
ccrsa_generate_fips186_key(size_t nbits, ccrsa_full_ctx_t fk,
                           size_t e_size, const void *eBytes,
                           struct ccrng_state *rng1, struct ccrng_state *rng2) CC_WARN_RESULT;

/* Construct RSA key from fix input in conformance with FIPS186-4 standard */
CC_NONNULL_TU((16)) CC_NONNULL((3, 5, 7, 9, 11, 13, 15))
int
ccrsa_make_fips186_key(size_t nbits,
                       const cc_size e_n, const cc_unit *e,
                       const cc_size xp1Len, const cc_unit *xp1, const cc_size xp2Len, const cc_unit *xp2,
                       const cc_size xpLen, const cc_unit *xp,
                       const cc_size xq1Len, const cc_unit *xq1, const cc_size xq2Len, const cc_unit *xq2,
                       const cc_size xqLen, const cc_unit *xq,
                       ccrsa_full_ctx_t fk,
                       cc_size *np, cc_unit *r_p,
                       cc_size *nq, cc_unit *r_q,
                       cc_size *nm, cc_unit *r_m,
                       cc_size *nd, cc_unit *r_d);

/*!
 * @brief ccrsa_sign_pss() generates RSASSA-PSS signature in PKCS1-V2 format
 *
 * note that in RSASSA-PSS, salt length is part of the signature as specified in ASN1
 * RSASSA-PSS-params ::= SEQUENCE {
 * hashAlgorithm      [0] HashAlgorithm      DEFAULT sha1,
 * maskGenAlgorithm   [1] MaskGenAlgorithm   DEFAULT mgf1SHA1,
 * saltLength         [2] INTEGER            DEFAULT 20,
 * trailerField       [3] TrailerField       DEFAULT trailerFieldBC
 *
 *
 * FIPS 186-4 for RSASSA-PSS:
 * .... Both signature schemes are approved for use, but additional constraints are imposed beyond those specified in PKCS #1 v2.1.....
 *
 * • If nlen = 1024 bits (i.e., 128 bytes), and the output length of the approved hash function output block is 512 bits (i.e., 64 bytes), then the length (in bytes) of the salt (sLen) shall satisfy 0 ≤ sLen ≤ hLen – 2,
 * • Otherwise, the length (in bytes) of the salt (sLen) shall satisfy 0 ≤ sLen ≤ hLen, where hLen is the length of the hash function output block (in bytes).
 *
 *
 * • CAVS test vectors are not very useful in the case of RSA-PSS, because they only validate the exponentiation part of the signature. See: http://csrc.nist.gov/groups/STM/cavp/documents/components/RSA2SP1VS.pdf
 *
 * @param	key              The RSA key
 * @param	hashAlgorithm    The hash algorithm used to generate mHash from the original message. It is also used inside the PSS encoding function. This is also the hash function to be used in the mask generation function (MGF)
 * @param   MgfHashAlgorithm The hash algorithm for thr mask generation function
 * @param   rng              Random number geberator to generate salt in PSS encoding
 * @param	saltSize          Intended length of the salt
 * @param   hSize             Length of message hash . Must be equal to hashAlgorithm->output_size
 * @param	mHash            The input that needs to be signed. This is the hash of message M with length of hLen
 *
 * @param   sig              The signature output
 * @param   sigSize           The length of generated signature in bytes, which equals the size of the RSA modulus.
 * @return                   0:ok, non-zero:error
 */
CC_NONNULL((2,3,5,7,8,9))
int ccrsa_sign_pss(ccrsa_full_ctx_t key,
                   const struct ccdigest_info* hashAlgorithm, const struct ccdigest_info* MgfHashAlgorithm,
                   size_t saltSize, struct ccrng_state *rng,
                   size_t hSize, const uint8_t *mHash,
                   size_t *sigSize, uint8_t *sig);

CC_NONNULL((2,3,5,7,9))
int ccrsa_verify_pss(ccrsa_pub_ctx_t key,
                     const struct ccdigest_info* di, const struct ccdigest_info* MgfDi,
                     size_t digestSize, const uint8_t *digest,
                     size_t sigSize, const uint8_t *sig,
                     size_t saltSize, bool *valid);

/*!
 @function   ccrsa_sign_pkcs1v15
 @abstract   RSA signature with PKCS#1 v1.5 format per PKCS#1 v2.2

 @param      key        Full key
 @param      oid        OID describing the type of digest passed in
 @param      digest_len Byte length of the digest
 @param      digest     Byte array of digest_len bytes containing the digest
 @param      sig_len    Pointer to the number of byte allocate for sig.
                        Output the exact size of the signature.
 @param      sig        Pointer to the allocated buffer of size *sig_len
                        for the output signature

 @result     0 iff successful.
 
  @discussion Null OID is a special case, required to support RFC 4346 where the padding
 is based on SHA1+MD5. In general it is not recommended to use a NULL OID,
 except when strictly required for interoperability

 */
CC_NONNULL_TU((1)) CC_NONNULL((4, 5, 6))
int ccrsa_sign_pkcs1v15(ccrsa_full_ctx_t key, const uint8_t *oid,
                        size_t digest_len, const uint8_t *digest,
                        size_t *sig_len, uint8_t *sig);


/*!
 @function   ccrsa_sign_pkcs1v15
 @abstract   RSA signature with PKCS#1 v1.5 format per PKCS#1 v2.2

 @param      key        Public key
 @param      oid        OID describing the type of digest passed in
 @param      digest_len Byte length of the digest
 @param      digest     Byte array of digest_len bytes containing the digest
 @param      sig_len    Number of byte of the signature sig.
 @param      sig        Pointer to the signature buffer of sig_len
 @param      valid      Output boolean, true if the signature is valid.

 @result     0 iff successful.
 
  @discussion Null OID is a special case, required to support RFC 4346 where the padding
 is based on SHA1+MD5. In general it is not recommended to use a NULL OID, 
 except when strictly required for interoperability
 */
CC_NONNULL_TU((1)) CC_NONNULL((4, 6, 7))
int ccrsa_verify_pkcs1v15(ccrsa_pub_ctx_t key, const uint8_t *oid,
                          size_t digest_len, const uint8_t *digest,
                          size_t sig_len, const uint8_t *sig,
                          bool *valid);

/*!
 @function   ccder_encode_rsa_pub_size
 @abstract   Calculate size of public key export format data package.
 
 @param      key        Public key
 
 @result     Returns size required for encoding.
 */

CC_NONNULL_TU((1))
size_t ccder_encode_rsa_pub_size(const ccrsa_pub_ctx_t key);

/*!
 @function   ccrsa_export_priv_pkcs1
 @abstract   Export a public key.
 
 @param      key        Public key
 @param      der        Beginning of output DER buffer
 @param      der_end    End of output DER buffer
 */

CC_NONNULL_TU((1)) CC_NONNULL((2)) CC_NONNULL((3))
uint8_t *ccder_encode_rsa_pub(const ccrsa_pub_ctx_t key, uint8_t *der, uint8_t *der_end);


/*!
 @function   ccder_encode_rsa_priv_size
 @abstract   Calculate size of full key exported in PKCS#1 format.
 
 @param      key        Full key
 
 @result     Returns size required for encoding.
 */

CC_NONNULL_TU((1))
size_t ccder_encode_rsa_priv_size(const ccrsa_full_ctx_t key);

/*!
 @function   ccder_encode_rsa_priv
 @abstract   Export a full key in PKCS#1 format.
 
 @param      key        Full key
 @param      der        Beginning of output DER buffer
 @param      der_end    End of output DER buffer
 */

CC_NONNULL_TU((1)) CC_NONNULL((2)) CC_NONNULL((3))
uint8_t *ccder_encode_rsa_priv(const ccrsa_full_ctx_t key, const uint8_t *der, uint8_t *der_end);

/*!
 @function   ccder_decode_rsa_pub_n
 @abstract   Calculate "n" for a public key imported from a data package.
        PKCS #1 format
 
 @param      der        Beginning of input DER buffer
 @param      der_end    End of input DER buffer
 
 @result the "n" of the RSA key that would result from the import.  This can be used
 to declare the key itself.
 */

CC_NONNULL((1)) CC_NONNULL((2))
cc_size ccder_decode_rsa_pub_n(const uint8_t *der, const uint8_t *der_end);

/*!
 @function   ccder_decode_rsa_pub
 @abstract   Import a public RSA key from a package in public key format.
        PKCS #1 format
 
 @param      key          Public key (n must be set)
 @param      der        Beginning of input DER buffer
 @param      der_end    End of input DER buffer
 
 @result     Key is initialized using the data in the public key message.
 */

CC_NONNULL_TU((1)) CC_NONNULL((2)) CC_NONNULL((3))
const uint8_t *ccder_decode_rsa_pub(const ccrsa_pub_ctx_t key, const uint8_t *der, const uint8_t *der_end);

/*!
 @function   ccder_decode_rsa_pub_x509_n
 @abstract   Calculate "n" for a public key imported from a data package in x509 format

 @param      der        Beginning of input DER buffer
 @param      der_end    End of input DER buffer

 @result the "n" of the RSA key that would result from the import.  This can be used
 to declare the key itself.
 */

CC_NONNULL((1)) CC_NONNULL((2))
cc_size ccder_decode_rsa_pub_x509_n(const uint8_t *der, const uint8_t *der_end);

/*!
 @function   ccder_decode_rsa_pub_x509
 @abstract   Import a public RSA key from a package in x509 format.

 @param      key          Public key (n must be set)
 @param      der        Beginning of input DER buffer
 @param      der_end    End of input DER buffer

 @result     Key is initialized using the data in the public key message.
 */

CC_NONNULL_TU((1)) CC_NONNULL((2)) CC_NONNULL((3))
const uint8_t *ccder_decode_rsa_pub_x509(const ccrsa_pub_ctx_t key, const uint8_t *der, const uint8_t *der_end);


/*!
 @function   ccder_decode_rsa_priv_n
 @abstract   Calculate "n" for a private key imported from a data package.
 
 @param      der        Beginning of input DER buffer
 @param      der_end    End of input DER buffer
 
 @result the "n" of the RSA key that would result from the import.  This can be used
 to declare the key itself.
 */

CC_NONNULL((1)) CC_NONNULL((2))
cc_size ccder_decode_rsa_priv_n(const uint8_t *der, const uint8_t *der_end);

/*!
 @function   ccder_decode_rsa_priv
 @abstract   Import a private RSA key from a package in PKCS#1 format.
 
 @param      key          Full key (n must be set)
 @param      der        Beginning of input DER buffer
 @param      der_end    End of input DER buffer
 
 @result     Key is initialized using the data in the public key message.
 */

CC_NONNULL_TU((1)) CC_NONNULL((2)) CC_NONNULL((3))
const uint8_t *ccder_decode_rsa_priv(const ccrsa_full_ctx_t key, const uint8_t *der, const uint8_t *der_end);

/*!
 @function   ccrsa_export_pub_size
 @abstract   Calculate size of public key exported data package.
 
 @param      key        Public key
 
 @result     Returns size required for encoding.
 */

CC_CONST CC_INLINE CC_NONNULL_TU((1))
size_t ccrsa_export_pub_size(const ccrsa_pub_ctx_t key) {
    return ccder_encode_rsa_pub_size(key);
}

/*!
 @function   ccrsa_export_pub
 @abstract   Export a public key in public key format.
 
 @param      key        Public key
 @param      out_len    Allocated size
 @param      out        Output buffer
 */

CC_NONNULL_TU((1)) CC_NONNULL((3))
int ccrsa_export_pub(const ccrsa_pub_ctx_t key, size_t out_len, uint8_t *out);
/*!
 @function   ccrsa_import_pub_n
 @abstract   Calculate "n" for a public key imported from a data package.
 
 @param      inlen        Length of public key package data
 @param      der          pointer to public key package data
 
 @result the "n" of the RSA key that would result from the import.  This can be used
 to declare the key itself.
 */

CC_CONST CC_INLINE CC_NONNULL((2))
cc_size ccrsa_import_pub_n(size_t inlen, const uint8_t *der) {
    cc_size size = ccder_decode_rsa_pub_x509_n(der, der + inlen);
    if(size == 0) {
        size = ccder_decode_rsa_pub_n(der, der + inlen);
    }
    return size;
}

/*!
 @function   ccrsa_import_pub
 @abstract   Import a public RSA key from a package in public key format.
 
 @param      key          Public key (n must be set)
 @param      inlen        Length of public key package data
 @param      der           pointer to public key package data
 
 @result     Key is initialized using the data in the public key message.
 */

CC_NONNULL_TU((1)) CC_NONNULL((3))
int ccrsa_import_pub(ccrsa_pub_ctx_t key, size_t inlen, const uint8_t *der);

/*!
 @function   ccrsa_export_priv_size
 @abstract   Calculate size of full key exported in PKCS#1 format.
 
 @param      key        Full key
 
 @result     Returns size required for encoding.
 */

CC_CONST CC_INLINE CC_NONNULL_TU((1))
size_t ccrsa_export_priv_size(const ccrsa_full_ctx_t key) {
    return ccder_encode_rsa_priv_size(key);
}

/*!
 @function   ccrsa_export_priv
 @abstract   Export a full key in PKCS#1 format.
 
 @param      key        Full key
 @param      out_len    Allocated size
 @param      out        Output buffer
 */

CC_CONST CC_INLINE CC_NONNULL_TU((1)) CC_NONNULL((3))
int ccrsa_export_priv(const ccrsa_full_ctx_t key, size_t out_len, uint8_t *out) {
    return (ccder_encode_rsa_priv(key, out, out+out_len) != out);
}

/*!
 @function   ccrsa_import_priv_n
 @abstract   Calculate size of full key exported in PKCS#1 format.
 
 @param      inlen        Length of PKCS#1 package data
 @param      der           pointer to PKCS#1 package data
 
 @result the "n" of the RSA key that would result from the import.  This can be used
 to declare the key itself.
 */

CC_CONST CC_INLINE CC_NONNULL((2))
cc_size ccrsa_import_priv_n(size_t inlen, const uint8_t *der) {
    return ccder_decode_rsa_priv_n(der, der + inlen);
}

/*!
 @function   ccrsa_import_priv
 @abstract   Import a full RSA key from a package in PKCS#1 format.
 
 @param      key          Full key (n must be set)
 @param      inlen        Length of PKCS#1 package data
 @param      der           pointer to PKCS#1 package data
 
 @result     Key is initialized using the data in the PKCS#1 message.
 */

CC_CONST CC_INLINE CC_NONNULL_TU((1)) CC_NONNULL((3))
int ccrsa_import_priv(ccrsa_full_ctx_t key, size_t inlen, const uint8_t *der) {
    return (ccder_decode_rsa_priv(key, der, der+inlen) == NULL);
}


CC_NONNULL_TU((1)) CC_NONNULL2
int ccrsa_get_pubkey_components(const ccrsa_pub_ctx_t pubkey, uint8_t *modulus, size_t *modulusLength, uint8_t *exponent, size_t *exponentLength);

CC_NONNULL_TU((1)) CC_NONNULL2
int ccrsa_get_fullkey_components(const ccrsa_full_ctx_t key, uint8_t *modulus, size_t *modulusLength, uint8_t *exponent, size_t *exponentLength,
                                 uint8_t *p, size_t *pLength, uint8_t *q, size_t *qLength);


/*!
 @function   ccrsa_dump_public_key
 @abstract   Print a rsa public key in the console (printf)

 @param      key          Public key
 */
void ccrsa_dump_public_key(ccrsa_pub_ctx_t key);

/*!
 @function   ccrsa_dump_full_key
 @abstract   Print a rsa private key in the console (printf)

 @param      key          Public key
 */
void ccrsa_dump_full_key(ccrsa_full_ctx_t key);

#endif /* _CORECRYPTO_CCRSA_H_ */
