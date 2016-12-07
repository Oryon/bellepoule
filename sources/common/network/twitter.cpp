#include "oauth/http_request.hpp"
#include "oauth/session.hpp"
#include "oauth/request_token.hpp"
#include "oauth/access_token.hpp"

#include "twitter.hpp"

namespace Net
{
  class VerifyCredentials : public Oauth::HttpRequest
  {
    public:
      VerifyCredentials (Oauth::Session *session)
        : Oauth::HttpRequest (session, GET, "VerifyCredentials")
      {
      }

    private:
      virtual ~VerifyCredentials ()
      {
      }

      const gchar *GetURL ()
      {
        return "https://api.twitter.com/1.1/account/verify_credentials.json";
      }
  };
}

namespace Net
{
  // --------------------------------------------------------------------------------
  Twitter::Twitter (Listener *listener)
    : Module ("twitter.glade")
  {
    _listener = listener;

    _state = OFF;

    _session = new Oauth::Session ("E7YgKcY2Yt9bHLxceaVBSg",
                                   "8HnMWMXOZgCrRE5VFILIlx0pQUuXIxkgd5aYh34rfg");

    //_session->SetToken       ("300716447-zFkdBm8f9uDKBG1j2fR6OjcVx4IWKFb7prgD7OUJ");
    //_session->SetTokenSecret ("SsEyYrg6PWapYCsLMDjMpl4R4o1t8KZlp5o1k1WH5HmYh");

    _listener->OnTwitterID (NULL);
  }

  // --------------------------------------------------------------------------------
  Twitter::~Twitter ()
  {
    _session->Release ();
  }

  // --------------------------------------------------------------------------------
  void Twitter::OnTwitterResponse (Oauth::HttpRequest *request,
                                   const gchar        *response)
  {
    if (response == NULL)
    {
      _state = OFF;
      _listener->OnTwitterID ("Network error!");
    }
    else if (g_strstr_len (response,
                           -1,
                           "errors"))
    {
      if ((_state == OFF) && (dynamic_cast <VerifyCredentials *> (request)))
      {
        _state = STARTUP;
        SendRequest (new Oauth::RequestToken (_session));
      }
      else
      {
        _state = OFF;
        _listener->OnTwitterID ("Access denied!");
      }
    }
    else
    {
      Oauth::RequestToken *request_token = dynamic_cast <Oauth::RequestToken *> (request);

      if (request_token)
      {
        gchar *pin_url = request_token->GetPinCodeUrl ();

        {
          GtkWidget *link = _glade->GetWidget ("pin_url");

          gtk_link_button_set_uri (GTK_LINK_BUTTON (link),
                                   pin_url);

          g_free (pin_url);
        }

        {
          GtkWidget *w = _glade->GetWidget ("request_token_dialog");

          switch (gtk_dialog_run (GTK_DIALOG (w)))
          {
            case GTK_RESPONSE_OK:
            {
              GtkEntry *entry = GTK_ENTRY (_glade->GetWidget ("pin_entry"));

              SendRequest (new Oauth::AccessToken (_session,
                                                   gtk_entry_get_text (entry)));

            }
            break;

            default:
            {
              _listener->OnTwitterID (NULL);
            }
            break;
          }

          gtk_widget_hide (w);
        }
      }
      else if (dynamic_cast <Oauth::AccessToken *> (request))
      {
        SendRequest (new VerifyCredentials (_session));
      }
      else
      {
        _state = ON;
        _listener->OnTwitterID ("(tagada@soinsoin)");
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Twitter::Use ()
  {
    Retain ();
  }

  // --------------------------------------------------------------------------------
  void Twitter::Drop ()
  {
    Release ();
  }

  // --------------------------------------------------------------------------------
  void Twitter::SendRequest (Oauth::HttpRequest *request)
  {
    TwitterUploader *uploader = new TwitterUploader (this);

    uploader->UpLoadRequest (request);

    request->Release ();
    uploader->Release ();
  }

  // --------------------------------------------------------------------------------
  void Twitter::SwitchOn ()
  {
    SendRequest (new VerifyCredentials (_session));
  }

  // --------------------------------------------------------------------------------
  void Twitter::SwitchOff ()
  {
    gtk_widget_hide (_glade->GetWidget ("twitter_dialog"));
  }
}
