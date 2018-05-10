# include <glib.h>
# include <gmime/gmime.h>
# include "utils/gmime/gmime-compat.h"

# include <string>

# include <boost/algorithm/string.hpp>

# include "astroid.hh"
# include "config.hh"
# include "crypto.hh"
# include "utils/address.hh"

namespace Astroid {
  Crypto::Crypto (ustring _protocol) {
    using std::endl;
    config       = astroid->config ("crypto");
    gpgpath      = ustring (config.get<std::string> ("gpg.path"));
    always_trust = config.get<bool> ("gpg.always_trust");
    throw_keyids = config.get<bool> ("gpg.throw_keyids");

    LOG (debug) << "crypto: gpg: " << gpgpath;

    protocol = _protocol.lowercase ();

    if ((protocol == "application/pgp-encrypted" ||
         protocol == "application/pgp-signature")) {

      isgpg = true;
      create_gpg_context ();

    } else {
      LOG (error) << "crypto: unsupported protocol: " << protocol;
      ready = false;
      return;
    }


    ready = true;
  }

  Crypto::~Crypto () {
    LOG (debug) << "crypto: deconstruct.";

    /* if (slist)        g_object_unref (slist); */
    /* if (rlist)        g_object_unref (rlist); */
    if (decrypt_res)  g_object_unref (decrypt_res);
    if (gpgctx)       g_object_unref (gpgctx);
  }

  GMimeObject * Crypto::decrypt_and_verify (GMimeObject * part) {
    using std::endl;
    LOG (debug) << "crypto: decrypting and verifiying..";
    decrypt_tried = true;

    if (!GMIME_IS_MULTIPART_ENCRYPTED (part)) {
      LOG (error) << "crypto: part is not encrypted.";
      return NULL;
    }

    GError *err = NULL;

    GMimeMultipartEncrypted * ep = GMIME_MULTIPART_ENCRYPTED (part);
    GMimeObject * dp = g_mime_multipart_encrypted_decrypt
	(ep, GMIME_DECRYPT_NONE, NULL, &decrypt_res, &err);

    /* GMimeDecryptResult and GMimeCertificates
     *
     * Only the certificates of the signature are fully populated, the certificates
     * listed in GMimeDecryptResult->recipients are only a list of the alleged
     * key ids of the receivers. These can be spoofed by the sender, or normally set to
     * 0x0 if the receivers should be anonymous (-R for gpg).
     *
     * It is not possible not have a certificate entry for a receiver, so it is always
     * possible to see how many recipients there are for a message.
     *
     * For decrypted messages the certificate only has the key_id field set, the rest
     * of the information has to be fetched manually (note that the key_id is for the
     * subkey that supports encryption - E). When the rest of the information is fetched
     * from an available public key we cannot be sure that this is really the recipient.
     *
     *
     * Encryption and Decryption:
     *
     * The option always_trust on the gpg context must be set if the local key is not
     * trusted (or considered valid). Otherwise it is not used for encryption-target.
     *
     * There is some confusion about the term 'trust' here since it normally means whether
     * you trust the keys signing of other keys, in this case it means 'is the key valid'
     * 'or is the key really the one from the recipient'.
     *
     */

    if (decrypt_res) {
      rlist = g_mime_decrypt_result_get_recipients (decrypt_res);
      slist = g_mime_decrypt_result_get_signatures (decrypt_res);

      for (int i = 0; i < g_mime_certificate_list_length (rlist); i++) {

        GMimeCertificate * ce = g_mime_certificate_list_get_certificate (rlist, i);

        const char * c = NULL;
        ustring fp = (c = g_mime_certificate_get_fingerprint (ce), c ? c : "");
        ustring nm = (c = g_mime_certificate_get_name (ce), c ? c : "");
        ustring em = (c = g_mime_certificate_get_email (ce), c ? c : "");
        ustring key = (c = g_mime_certificate_get_key_id (ce), c ? c : "");

        LOG (debug) << "cr: encrypted for: " << nm << "(" << em << ") [" << fp << "] [" << key << "]";
      }
    }

    if (dp == NULL) {
      LOG (error) << "crypto: failed to decrypt message: " << err->message;
      decrypted = false;
      decrypt_error = err->message;

    } else {
      LOG (info) << "crypto: successfully decrypted message.";
      decrypted = true;

      verify_tried = (slist != NULL);
      verified = verify_signature_list (slist);
    }

    return dp;
  }

  GMimeMessage * Crypto::decrypt_message (GMimeMessage * in) {
    GMimeStream * ins = GMIME_STREAM (g_mime_stream_mem_new ());

    g_mime_object_write_to_stream (GMIME_OBJECT(in), NULL, ins);
    g_mime_stream_seek (ins, 0, GMIME_STREAM_SEEK_SET);

    GError * err = NULL;
    GMimeStream * outs = GMIME_STREAM (g_mime_stream_mem_new ());

    GMimeDecryptResult * decrypt_res = g_mime_crypto_context_decrypt (gpgctx, GMIME_DECRYPT_NONE, NULL,
        ins, outs, &err);

    g_mime_stream_flush (outs);
    g_mime_stream_seek (outs, 0, GMIME_STREAM_SEEK_SET);

    g_object_unref (ins);

    /* check if message */
    GMimeParser * parser = g_mime_parser_new_with_stream (outs);
    GMimeMessage * dp = g_mime_parser_construct_message (parser, NULL);
    g_object_unref (parser);
    g_object_unref (outs);

    if (decrypt_res) {
      rlist = g_mime_decrypt_result_get_recipients (decrypt_res);
      slist = g_mime_decrypt_result_get_signatures (decrypt_res);

      for (int i = 0; i < g_mime_certificate_list_length (rlist); i++) {

        GMimeCertificate * ce = g_mime_certificate_list_get_certificate (rlist, i);

        const char * c = NULL;
        ustring fp = (c = g_mime_certificate_get_fingerprint (ce), c ? c : "");
        ustring nm = (c = g_mime_certificate_get_name (ce), c ? c : "");
        ustring em = (c = g_mime_certificate_get_email (ce), c ? c : "");
        ustring key = (c = g_mime_certificate_get_key_id (ce), c ? c : "");

        LOG (debug) << "cr: encrypted for: " << nm << "(" << em << ") [" << fp << "] [" << key << "]";
      }
    }

    if (dp == NULL) {
      LOG (error) << "crypto: failed to decrypt message: " << err->message;
      decrypted = false;
      decrypt_error = err->message;

    } else {
      LOG (info) << "crypto: successfully decrypted message.";
      decrypted = true;

      verify_tried = (slist != NULL);
      verified = verify_signature_list (slist);
    }

    return dp;
  }

  bool Crypto::verify_signature (GMimeObject * mo) {
    GError * err = NULL;

    verify_tried = true;

    slist = g_mime_multipart_signed_verify (GMIME_MULTIPART_SIGNED(mo), GMIME_VERIFY_NONE, &err);

    verified = verify_signature_list (slist);

    return verified;
  }

  bool Crypto::verify_signature_list (GMimeSignatureList * list) {
    if (list == NULL) return false;

    bool res = g_mime_signature_list_length (list) > 0;

    for (int i = 0; i < g_mime_signature_list_length (list); i++) {
      GMimeSignature * s = g_mime_signature_list_get_signature (list, i);

      res &= g_mime_signature_status_good(g_mime_signature_get_status (s));
    }

    return res;
  }

  bool Crypto::encrypt (GMimeObject * mo, bool sign, ustring userid, InternetAddress * from, ustring to, GMimeMultipartEncrypted ** out, GError ** err)
  {
    return encrypt (mo, sign, userid, from, AddressList (to), out, err);
  }

  bool Crypto::encrypt (GMimeObject * mo, bool sign, ustring userid, InternetAddress * from, AddressList to, GMimeMultipartEncrypted ** out, GError ** err)
  {

    /* build receipients */
    AddressList recp  = to + Address (from);
    recp.remove_duplicates ();

    GPtrArray * recpa = g_ptr_array_sized_new (recp.size ());

    std::vector<ustring> ur;

    for (Address &a : recp.addresses) {
      ur.push_back (a.email ());
    }

    LOG (debug) << "cr: encrypting for: ";
    for (ustring &u : ur) {
      g_ptr_array_add (recpa, (gpointer) u.c_str ());
      LOG (debug) << u << " ";
    }

    *out = g_mime_multipart_encrypted_encrypt (
        gpgctx,
        mo,
        sign,
        userid.c_str (),
# if (GMIME_MAJOR_VERSION >= 3)
        (throw_keyids ? GMIME_ENCRYPT_THROW_KEYIDS : GMIME_ENCRYPT_NONE), // only in Gmime 3
# else
        GMIME_ENCRYPT_NONE,
# endif
        recpa,
        err);


    g_ptr_array_free (recpa, true);

    if (*out != NULL) {
      LOG (debug) << "crypto: successfully encrypted message.";
      return true;
    } else {
      LOG (debug) << "crypto: failed to encrypt message: " << (*err)->message;
      return false;
    }
  }

  bool Crypto::sign (GMimeObject * mo, ustring userid, GMimeMultipartSigned ** out, GError ** err) {
    *out = g_mime_multipart_signed_sign (
        gpgctx,
        mo,
        userid.c_str (),
        err);

    if (*out != NULL) {
      LOG (debug) << "crypto: successfully signed message.";
      return true;
    } else {
      LOG (debug) << "crypto: failed to sign message: " << (*err)->message;
      return false;
    }
  }

  bool Crypto::create_gpg_context () {

    if (!astroid->in_test ()) {

# if (GMIME_MAJOR_VERSION < 3)
      gpgctx = g_mime_gpg_context_new(NULL, gpgpath.length() ? gpgpath.c_str () : "gpg");
      g_mime_gpg_context_set_use_agent ((GMimeGpgContext *) gpgctx, TRUE);
      g_mime_gpg_context_set_always_trust ((GMimeGpgContext *) gpgctx, always_trust);
# else
      gpgctx = g_mime_gpg_context_new ();
# endif
    } else {

      LOG (debug) << "crypto: in test";
# if (GMIME_MAJOR_VERSION < 3)
      gpgctx = g_mime_gpg_context_new(NULL, "gpg");
      g_mime_gpg_context_set_use_agent ((GMimeGpgContext *) gpgctx, TRUE);
      g_mime_gpg_context_set_always_trust ((GMimeGpgContext *) gpgctx, TRUE);
# else
      gpgctx = g_mime_gpg_context_new ();
# endif
    }

    if (! gpgctx) {
      LOG (error) << "crypto: failed to create gpg context.";
      return false;
    }


    return true;
  }

  ustring Crypto::get_md5_digest (ustring str) {
    std::string cs = Glib::Checksum::compute_checksum (Glib::Checksum::ChecksumType::CHECKSUM_MD5, str);

    return cs;
  }

  gssize Crypto::get_md5_length () {
    return Glib::Checksum::get_length (Glib::Checksum::ChecksumType::CHECKSUM_MD5);
  }

  refptr<Glib::Bytes> Crypto::get_md5_digest_b (ustring str) {
    if (str.empty ()) {
      guint8 buffer[get_md5_length ()];
      return Glib::Bytes::create (buffer, get_md5_length ());
    }

    guint8 buffer[get_md5_length ()];
    gsize  len = get_md5_length ();

    Glib::Checksum chk (Glib::Checksum::ChecksumType::CHECKSUM_MD5);
    chk.update (str);

    chk.get_digest (buffer, &len);

    return Glib::Bytes::create (buffer, len);
  }
}


