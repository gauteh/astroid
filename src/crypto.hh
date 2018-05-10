# pragma once

# include <gmime/gmime.h>
# include <boost/property_tree/ptree.hpp>

# include "astroid.hh"
# include "utils/address.hh"
# include "proto.hh"

using boost::property_tree::ptree;

namespace Astroid {
  class Crypto : public Glib::Object {
    public:
      Crypto (ustring protocol);
      ~Crypto ();

      bool ready = false;
      bool isgpg = false;

      GMimeObject * decrypt_and_verify (GMimeObject * mo);
      GMimeMessage * decrypt_message (GMimeMessage * in);

      bool verify_signature (GMimeObject * mo);

      bool encrypt (GMimeObject * mo,
                    bool sign,
                    ustring userid,
                    InternetAddress * from,
                    ustring to,
                    GMimeMultipartEncrypted ** e,
                    GError **);
      bool encrypt (GMimeObject * mo,
                    bool sign,
                    ustring userid,
                    InternetAddress * from,
                    AddressList to,
                    GMimeMultipartEncrypted ** e,
                    GError **);

      bool sign (GMimeObject * mo, ustring userid, GMimeMultipartSigned ** s, GError **);


      bool decrypted        = false;
      bool verified         = false; /* signature ok */
      bool verify_tried     = false;
      bool decrypt_tried    = false;
      ustring decrypt_error = "";

      GMimeDecryptResult *   decrypt_res = NULL;
      GMimeSignatureList *   slist       = NULL;
      GMimeCertificateList * rlist       = NULL;

    private:
      bool create_gpg_context ();
      GMimeCryptoContext * gpgctx = NULL;

      ustring protocol;
      ustring gpgpath;
      bool    always_trust = false;
      bool    throw_keyids = true;
      ptree   config;

      bool verify_signature_list (GMimeSignatureList *);

    public:
      static ustring  get_md5_digest (ustring str);
      static gssize   get_md5_length ();
      static refptr<Glib::Bytes> get_md5_digest_b (ustring str);
  };

}

