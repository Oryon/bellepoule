#include "oauth/http_request.hpp"
#include "oauth/session.hpp"
#include "oauth/request_token.hpp"
#include "oauth/access_token.hpp"
#include "twitter_uploader.hpp"

#include "twitter.hpp"

namespace Net
{
  class Toto : public Oauth::HttpRequest
  {
    public:
      Toto (Oauth::Session *session)
        : Oauth::HttpRequest (session, "GET", "Toto")
      {
      }

    private:
      virtual ~Toto ()
      {
      }

      const gchar *GetURL ()
      {
        return "https://api.twitter.com/1.1/account/verify_credentials.json";
      }
  };

  // --------------------------------------------------------------------------------
  Twitter::Twitter ()
    : Object ("Twitter")
  {
    _session = new Oauth::Session ("E7YgKcY2Yt9bHLxceaVBSg",
                                   "8HnMWMXOZgCrRE5VFILIlx0pQUuXIxkgd5aYh34rfg");

    _session->SetToken       ("300716447-zFkdBm8f9uDKBG1j2fR6OjcVx4IWKFb7prgD7OUJ");
    _session->SetTokenSecret ("SsEyYrg6PWapYCsLMDjMpl4R4o1t8KZlp5o1k1WH5HmYh");
  }

  // --------------------------------------------------------------------------------
  Twitter::~Twitter ()
  {
    _session->Release ();
  }

  // --------------------------------------------------------------------------------
  void Twitter::Open ()
  {
#if 0
    {
      gchar *pin = g_new0 (gchar, 1024);

      // request_token
      {
        gchar               *pin_url;
        Oauth::RequestToken *request_token = new Oauth::RequestToken (_session);

        PostOauthRequest (request_token);

        pin_url = request_token->GetPinCodeUrl ();
        printf ("%s\n", pin_url);
        g_free (pin_url);

        gets (pin);

        request_token->Release ();
      }

      // access_token
      {
        Oauth::AccessToken *access_token = new Oauth::AccessToken (_session,
                                                                   pin);

        PostOauthRequest (access_token);
        access_token->Release ();
      }
    }
#endif

    // verify_credentials
    {
      Toto            *toto     = new Toto (_session);
      TwitterUploader *uploader = new TwitterUploader ();

      uploader->UpLoadRequest (toto);

      toto->Release     ();
      uploader->Release ();
    }
  }

  // --------------------------------------------------------------------------------
  void Twitter::Close ()
  {
  }
}
