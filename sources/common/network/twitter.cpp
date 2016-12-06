#include <stdio.h>
#include <string.h>
#include <curl/curl.h>

#include "oauth/http_request.hpp"
#include "oauth/session.hpp"
#include "oauth/request_token.hpp"
#include "oauth/access_token.hpp"

#include "twitter.hpp"

namespace Net
{
  static CURL *curl;

  // --------------------------------------------------------------------------------
  int OnUpLoadTrace (CURL          *handle,
                     curl_infotype  type,
                     char          *data,
                     size_t         size,
                     gpointer      *extra)
  {
    if (type == CURLINFO_TEXT)
    {
      g_print (BLUE "[Uploader] " ESC);
    }
    else if (type == CURLINFO_HEADER_IN)
    {
      g_print (GREEN "--CURLINFO_HEADER_IN------\n" ESC);
    }
    else if (type == CURLINFO_HEADER_OUT)
    {
      g_print (GREEN "--CURLINFO_HEADER_OUT-----\n" ESC);
    }
    else if (type == CURLINFO_DATA_IN)
    {
      g_print (GREEN "--CURLINFO_DATA_IN--------\n" ESC);
    }
    else if (type == CURLINFO_DATA_OUT)
    {
      g_print (GREEN "--CURLINFO_DATA_OUT-------\n" ESC);
    }

    if (data && size)
    {
      gchar *printable_buffer = g_strndup (data, size);

      g_print ("%s", printable_buffer);
      if (printable_buffer[size-1] != '\n')
      {
        g_print ("\n");
      }
      g_free (printable_buffer);
    }

    return 0;
  }

  // --------------------------------------------------------------------------------
  struct Response
  {
    gchar  *message;
    size_t  size;

    Response ()
    {
      message = g_new (gchar, 1);
      size    = 0;
    }

    ~Response ()
    {
      g_free (message);
    }
  };

  static size_t OnResponse (void     *contents,
                            size_t    size,
                            size_t    nmemb,
                            Response *response)
  {
    size_t realsize = size * nmemb;

    response->message = (gchar *) g_realloc (response->message,
                                             response->size + realsize + 1);

    memcpy (&(response->message[response->size]),
            contents,
            realsize);

    response->size += realsize;
    response->message[response->size] = 0;

    return realsize;
  }

  // --------------------------------------------------------------------------------
  void PostOauthRequest (Oauth::HttpRequest *request)
  {
    CURLcode           res;
    struct curl_slist *header_list = NULL;

    Response *response = new Response ();

    curl_easy_setopt (curl, CURLOPT_SSL_VERIFYPEER, 0);
    curl_easy_setopt (curl, CURLOPT_CUSTOMREQUEST,  NULL);
    curl_easy_setopt (curl, CURLOPT_ENCODING,       "");

    curl_easy_setopt (curl, CURLOPT_HTTPGET,        1);
    curl_easy_setopt (curl, CURLOPT_URL,            request->GetURL ());

    curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, OnResponse);
    curl_easy_setopt (curl, CURLOPT_WRITEDATA,     response);

    curl_easy_setopt (curl, CURLOPT_DEBUGFUNCTION, OnUpLoadTrace);
    curl_easy_setopt (curl, CURLOPT_DEBUGDATA,     NULL);
    curl_easy_setopt (curl, CURLOPT_VERBOSE,       0L);

    {
      gchar *header = request->GetHeader ();

      header_list = curl_slist_append (header_list, header);
      curl_easy_setopt (curl, CURLOPT_HTTPHEADER, header_list);

      g_free (header);
    }

    res = curl_easy_perform (curl);
    if (res != CURLE_OK)
    {
      printf ("curl_easy_perform() failed: %s\n",
              curl_easy_strerror (res));
    }
    else
    {
      request->ParseResponse (response->message);
    }

    if (header_list)
    {
      curl_slist_free_all (header_list);
    }

    delete (response);
  }

  // --------------------------------------------------------------------------------
  Twitter::Twitter ()
    : Object ("Twitter")
  {
    _session = new Oauth::Session ("E7YgKcY2Yt9bHLxceaVBSg",
                                   "8HnMWMXOZgCrRE5VFILIlx0pQUuXIxkgd5aYh34rfg");

  }

  // --------------------------------------------------------------------------------
  Twitter::~Twitter ()
  {
    _session->Release ();
  }

  // --------------------------------------------------------------------------------
  void Twitter::Open ()
  {
    gchar *pin = g_new0 (gchar, 1024);

    curl = curl_easy_init ();

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

    curl_easy_cleanup (curl);
  }

  // --------------------------------------------------------------------------------
  void Twitter::Close ()
  {
  }
}
