// Copyright (C) 2009 Yannick Le Roux.
// This file is part of BellePoule.
//
//   BellePoule is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   BellePoule is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with BellePoule.  If not, see <http://www.gnu.org/licenses/>.

#include <stdlib.h>
#include <curl/curl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef WINDOWS_TEMPORARY_PATCH
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#endif

#include "util/attribute.hpp"
#include "util/global.hpp"
#include "util/module.hpp"
#include "util/partner.hpp"
#include "network/downloader.hpp"
#include "language.hpp"
#include "weapon.hpp"
#include "version.h"

#include "application.hpp"

const gchar *Application::ANNOUNCE_GROUP = "225.0.0.35";

// --------------------------------------------------------------------------------
Application::Application (const gchar   *config_file,
                          guint          http_port,
                          int           *argc,
                          char        ***argv)
{
  _language    = NULL;
  _main_module = NULL;
  _granted     = FALSE;

  // g_mem_set_vtable (glib_mem_profiler_table);

  // Init
  {
    {
      gchar *binary_dir;

      {
        gchar *program = g_find_program_in_path ((*argv)[0]);

        binary_dir = g_path_get_dirname (program);
        g_free (program);
      }

#ifdef DEBUG
      g_log_set_default_handler (LogHandler,
                                 NULL);
      Global::_share_dir = g_build_filename (binary_dir, "..", "..", "..", NULL);
#else
      {
        gchar *basename = g_path_get_basename ((*argv)[0]);

        if (strstr (basename, ".exe"))
        {
          g_free (basename);
          basename = g_strdup ("bellepoule");
        }

        Global::_share_dir = g_build_filename (binary_dir, "..", "share", basename, NULL);
        g_free (basename);
      }
#endif

      g_free (binary_dir);
    }

    if (Global::_share_dir)
    {
      gchar *gtkrc = g_build_filename (Global::_share_dir, "resources", "gtkrc", NULL);

      gtk_rc_add_default_file (gtkrc);
      g_free (gtkrc);
    }

    {
      gtk_init (argc, argv);

      g_type_class_unref (g_type_class_ref (GTK_TYPE_IMAGE_MENU_ITEM));
      g_object_set (gtk_settings_get_default (), "gtk-menu-images", TRUE, NULL);
    }

    //Object::Track ("Player");
  }

#if GTK_MAJOR_VERSION < 3
  gtk_about_dialog_set_url_hook (AboutDialogActivateLinkFunc,
                                 NULL,
                                 NULL);
#endif

  curl_global_init (CURL_GLOBAL_ALL);

  Global::_user_config = new UserConfig (config_file);

  {
    _version_downloader = new Net::Downloader ("_version_downloader",
                                               OnLatestVersionReceived,
                                               this);
    _version_downloader->Start ("http://betton.escrime.free.fr/documents/BellePoule/latest.html");
  }

  _http_server = new Net::HttpServer (this,
                                      HttpPostCbk,
                                      HttpGetCbk,
                                      http_port);
}

// --------------------------------------------------------------------------------
Application::~Application ()
{
  AttributeDesc::Cleanup ();

  _http_server->Release ();

  _language->Release ();

  if (_version_downloader)
  {
    _version_downloader->Kill    ();
    _version_downloader->Release ();
  }

  g_free (Global::_share_dir);

  Global::_user_config->Release ();

  curl_global_cleanup ();
}

// --------------------------------------------------------------------------------
void Application::Prepare ()
{
  _language = new Language ();

  // Weapon
  {
    new Weapon ("Educ",  "EE");
    new Weapon ("Foil",  "F");
    new Weapon ("Epée",  "E");
    new Weapon ("Sabre", "S");
  }

  // Attributes definition
  {
    AttributeDesc *desc;

    desc = AttributeDesc::Declare (G_TYPE_INT, "ref", "ID", gettext ("ref"));
    desc->_rights = AttributeDesc::PRIVATE;

    desc = AttributeDesc::Declare (G_TYPE_INT, "final_rank", "Classement", gettext ("place"));

    desc = AttributeDesc::Declare (G_TYPE_STRING, "team", "Equipe", gettext ("team"));
    desc->_uniqueness = AttributeDesc::NOT_SINGULAR;
    desc->AddDiscreteValues (NULL, NULL, NULL, NULL);
    desc->EnableSorting ();

    desc = AttributeDesc::Declare (G_TYPE_STRING, "name", "Nom", gettext ("last name"));

    desc = AttributeDesc::Declare (G_TYPE_STRING, "first_name", "Prenom", gettext ("first name"));

    desc = AttributeDesc::Declare (G_TYPE_STRING, "birth_date", "DateNaissance", gettext ("date of birth"));
    desc->_compare_func = (GCompareFunc) CompareDate;

    desc = AttributeDesc::Declare (G_TYPE_STRING, "gender", "Sexe", gettext ("gender"));
    desc->_uniqueness         = AttributeDesc::NOT_SINGULAR;
    desc->_free_value_allowed = FALSE;
    desc->_favorite_look      = AttributeDesc::GRAPHICAL;
    desc->AddDiscreteValues ("M", gettext ("Male"), "resources/glade/male.png",
                             "F", gettext ("Female"), "resources/glade/female.png", NULL);

    desc = AttributeDesc::Declare (G_TYPE_STRING, "country", "Nation", gettext ("country"));
    desc->_uniqueness = AttributeDesc::NOT_SINGULAR;
    desc->_favorite_look = AttributeDesc::GRAPHICAL;
    desc->AddDiscreteValueSelector ("countries");
    AttributeDesc::AddSwappable (desc);

    desc = AttributeDesc::Declare (G_TYPE_STRING, "league", "Ligue", gettext ("league"));
    desc->_favorite_look = AttributeDesc::GRAPHICAL;
    desc->_uniqueness = AttributeDesc::NOT_SINGULAR;
    desc->AddLocalizedDiscreteValues ("ligues");
    AttributeDesc::AddSwappable (desc);

    desc = AttributeDesc::Declare (G_TYPE_STRING, "club", "Club", gettext ("club"));
    desc->_uniqueness = AttributeDesc::NOT_SINGULAR;
    desc->AddLocalizedDiscreteValues ("clubs");
    AttributeDesc::AddSwappable (desc);

    desc = AttributeDesc::Declare (G_TYPE_STRING, "licence", "Licence", gettext ("licence"));

    desc = AttributeDesc::Declare (G_TYPE_STRING, "IP", "IP", gettext ("IP address"));
    desc->_rights = AttributeDesc::PRIVATE;

    desc = AttributeDesc::Declare (G_TYPE_INT, "participation_rate", "Activite", gettext ("rate"));
    desc->_persistency    = AttributeDesc::NOT_PERSISTENT;
    desc->_favorite_look  = AttributeDesc::GRAPHICAL;
    desc->_rights         = AttributeDesc::PRIVATE;

    desc = AttributeDesc::Declare (G_TYPE_STRING, "level", "Categorie", gettext ("level"));
    desc->_uniqueness = AttributeDesc::NOT_SINGULAR;

    desc = AttributeDesc::Declare (G_TYPE_INT, "ranking", "Ranking", gettext ("ranking"));
    desc->_compare_func = (GCompareFunc) CompareRanking;

    desc = AttributeDesc::Declare (G_TYPE_BOOLEAN, "attending", "Presence", gettext ("presence"));
    desc->_uniqueness    = AttributeDesc::NOT_SINGULAR;
    desc->_favorite_look = AttributeDesc::GRAPHICAL;
    desc->_persistency   = AttributeDesc::NOT_PERSISTENT;

    desc = AttributeDesc::Declare (G_TYPE_BOOLEAN, "exported", "Exporte", gettext ("exported"));
    desc->_favorite_look = AttributeDesc::GRAPHICAL;

    desc = AttributeDesc::Declare (G_TYPE_STRING, "global_status", "Statut", gettext ("global status"));
    desc->_scope  = AttributeDesc::GLOBAL;
    desc->_rights = AttributeDesc::PRIVATE;
    desc->_favorite_look = AttributeDesc::GRAPHICAL;
    desc->AddDiscreteValues ("Q", gettext ("Qualified"),     "resources/glade/normal.png",
                             "N", gettext ("Not qualified"), "resources/glade/exit.png",
                             "A", gettext ("Withdrawal"),    "resources/glade/ambulance.png",
                             "E", gettext ("Excluded"),      "resources/glade/black_card.png",
                             "F", gettext ("Forfeit"),       "resources/glade/normal.png", NULL);

    desc = AttributeDesc::Declare (G_TYPE_STRING, "status", "Statut", gettext ("status"));
    desc->_scope = AttributeDesc::LOCAL;
    desc->_favorite_look = AttributeDesc::GRAPHICAL;
    desc->AddDiscreteValues ("Q", gettext ("Qualified"),     "resources/glade/normal.png",
                             "N", gettext ("Not qualified"), "resources/glade/exit.png",
                             "A", gettext ("Withdrawal"),    "resources/glade/ambulance.png",
                             "E", gettext ("Excluded"),      "resources/glade/black_card.png",
                             "F", gettext ("Forfeit"),       "resources/glade/normal.png", NULL);

    // Not persistent data
    {
      desc = AttributeDesc::Declare (G_TYPE_INT, "pool_nr", "pool_nr", gettext ("pool #"));
      desc->_persistency = AttributeDesc::NOT_PERSISTENT;
      desc->_scope       = AttributeDesc::LOCAL;

      desc = AttributeDesc::Declare (G_TYPE_INT, "victories_count", "victories_count", gettext ("Victories"));
      desc->_persistency = AttributeDesc::NOT_PERSISTENT;
      desc->_scope       = AttributeDesc::LOCAL;

      desc = AttributeDesc::Declare (G_TYPE_INT, "bouts_count", "bouts_count", gettext ("Bouts"));
      desc->_persistency = AttributeDesc::NOT_PERSISTENT;
      desc->_scope       = AttributeDesc::LOCAL;

      desc = AttributeDesc::Declare (G_TYPE_INT, "victories_ratio", "victories_ratio", gettext ("Vict./Bouts (‰)"));
      desc->_persistency = AttributeDesc::NOT_PERSISTENT;
      desc->_scope       = AttributeDesc::LOCAL;

      desc = AttributeDesc::Declare (G_TYPE_INT, "indice", "indice", gettext ("index"));
      desc->_persistency = AttributeDesc::NOT_PERSISTENT;
      desc->_scope       = AttributeDesc::LOCAL;

      desc = AttributeDesc::Declare (G_TYPE_INT, "HS", "HS", gettext ("Hits scored"));
      desc->_persistency = AttributeDesc::NOT_PERSISTENT;
      desc->_scope       = AttributeDesc::LOCAL;

      desc = AttributeDesc::Declare (G_TYPE_INT, "rank", "rank", gettext ("place"));
      desc->_persistency = AttributeDesc::NOT_PERSISTENT;
      desc->_rights      = AttributeDesc::PRIVATE;
      desc->_scope       = AttributeDesc::LOCAL;

      desc = AttributeDesc::Declare (G_TYPE_INT, "stage_start_rank", "stage_start_rank", gettext ("Round start rank"));
      desc->_persistency = AttributeDesc::NOT_PERSISTENT;
      desc->_rights      = AttributeDesc::PRIVATE;
      desc->_scope       = AttributeDesc::LOCAL;

      desc = AttributeDesc::Declare (G_TYPE_BOOLEAN, "promoted", "promoted", gettext ("promoted"));
      desc->_persistency   = AttributeDesc::NOT_PERSISTENT;
      desc->_scope         = AttributeDesc::LOCAL;
      desc->_favorite_look = AttributeDesc::GRAPHICAL;

      desc = AttributeDesc::Declare (G_TYPE_STRING, "availability", "Disponibilite", gettext ("availability"));
      desc->_persistency    = AttributeDesc::NOT_PERSISTENT;
      desc->_rights         = AttributeDesc::PRIVATE;
      desc->_scope          = AttributeDesc::GLOBAL;
      desc->_favorite_look  = AttributeDesc::GRAPHICAL;
      desc->AddDiscreteValues ("Busy",   gettext ("Busy"),   (gchar *) GTK_STOCK_EXECUTE,
                               "Absent", gettext ("Absent"), (gchar *) GTK_STOCK_CLOSE,
                               "Free",   gettext ("Free"),   (gchar *) GTK_STOCK_APPLY, NULL);

      desc = AttributeDesc::Declare (G_TYPE_STRING, "connection", "Connection", gettext ("connection"));
      desc->_persistency    = AttributeDesc::NOT_PERSISTENT;
      desc->_rights         = AttributeDesc::PRIVATE;
      desc->_scope          = AttributeDesc::GLOBAL;
      desc->_favorite_look  = AttributeDesc::GRAPHICAL;
      desc->AddDiscreteValues ("Broken",  gettext ("Broken"),  "resources/glade/red.png",
                               "Waiting", gettext ("Waiting"), "resources/glade/orange.png",
                               "OK",      gettext ("OK"),      "resources/glade/green.png",
                               "Manual",  gettext ("Manual"),  (gchar *) GTK_STOCK_EDIT, NULL);

    }

    AttributeDesc::SetCriteria ("CSV ready",
                                IsCsvReady);
  }
}

// --------------------------------------------------------------------------------
void Application::Start (int    argc,
                         char **argv)
{
  _language->Populate (GTK_MENU_ITEM  (_main_module->GetGObject ("locale_menuitem")),
                       GTK_MENU_SHELL (_main_module->GetGObject ("locale_menu")));

  {
    GtkWidget *w       = GTK_WIDGET (_main_module->GetGObject ("about_dialog"));
    gchar     *version = g_strdup_printf ("V%s.%s%s", VERSION, VERSION_REVISION, VERSION_MATURITY);

    gtk_about_dialog_set_version (GTK_ABOUT_DIALOG (w),
                                  (const gchar *) version);
    g_free (version);
  }

  {
    GtkWidget   *w         = GTK_WIDGET (_main_module->GetGObject ("about_dialog"));
    const gchar *authors[] = {"Florence Blanchard",
                              "Laurent Bonnot",
                              "Tony (ajs New Mexico)",
                              "Emmanuel Chaix",
                              "Julien Diaz",
                              "Olivier Larcher",
                              "Yannick Le Roux",
                              "Jean-Pierre Mahé",
                              "Pierre Moro",
                              "Killian Poulet",
                              "Michel Relet",
                              "Vincent Rémy",
                              "Tina Schliemann",
                              "Dom Walden",
                              "Claude Simonnot",
                              "Sébastien Vermandel",
                              NULL};

    gtk_about_dialog_set_authors (GTK_ABOUT_DIALOG (w),
                                  authors);
  }

  {
    GtkWidget *w           = GTK_WIDGET (_main_module->GetGObject ("about_dialog"));
    gchar     *translators = g_strdup_printf ("Julien Diaz           (German)\n"
                                              "Tina Schliemann       (German)\n"
                                              "Michael Weber         (German)\n"
                                              "Dom Walden            (English\n"
                                              "Aureliano Martini     (Italian)\n"
                                              "Jihwan Cho            (Korean)\n"
                                              "Marijn Somers         (Dutch)\n"
                                              "Werner Huysmans       (Dutch)\n"
                                              "Farid Elkhalki        (Japanese)\n"
                                              "Jean-François Brun    (Japanese)\n"
                                              "Alexis Pigeon         (Spanish)\n"
                                              "Paulo Morales         (Spanish)\n"
                                              "Ignacio Gil           (Spanish)\n"
                                              "Eduardo Alberto Calvo (Spanish, Portuguese)\n"
                                              "Rui Pedro Vieira      (Portuguese)\n"
                                              "Mohamed Rebai         (Arabic)\n"
                                              "Mikael Hiort af Ornäs (Swedish)\n"
                                              "Sergev Makhtanov      (Russian)");

    gtk_about_dialog_set_translator_credits (GTK_ABOUT_DIALOG (w),
                                             translators);
    g_free (translators);
  }

  {
    GtkWidget *w = _main_module->GetRootWidget ();

    gtk_widget_show_all (w);
  }

  gtk_widget_hide (GTK_WIDGET (_main_module->GetGObject ("update_menuitem")));
}

// --------------------------------------------------------------------------------
gboolean Application::MulticastAvailability (Application *application)
{
  struct sockaddr_in  addr;
  int                 fd;

  if ((fd = socket (AF_INET,
                    SOCK_DGRAM,
                    0)) < 0)
  {
    g_warning ("socket: %s", strerror (errno));
    return FALSE;
  }

  {
    memset (&addr,
            0,
            sizeof(addr));

    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = inet_addr (ANNOUNCE_GROUP);
    addr.sin_port        = htons     (ANNOUNCE_PORT);
  }

  {
    char *message = g_strdup_printf ("HallManager:%d", application->_http_server->GetPort ());

    if (sendto (fd,
                message,
                strlen (message) + 1,
                0,
                (struct sockaddr *) &addr,
                sizeof(addr)) < 0)
    {
      g_warning ("sendto: %s", strerror (errno));
      g_free (message);
      return FALSE;
    }

    g_free (message);
  }

  return (application->_granted == FALSE);
}

// --------------------------------------------------------------------------------
void Application::AnnounceAvailability ()
{
  g_timeout_add_seconds (5,
                         (GSourceFunc) MulticastAvailability,
                         this);
}

// --------------------------------------------------------------------------------
void Application::ListenToAnnouncement ()
{
  GError *error = NULL;

  if (!g_thread_create ((GThreadFunc) AnnoucementListener,
                        this,
                        FALSE,
                        &error))
  {
    g_warning ("Failed to create AnnoucementListener thread: %s\n", error->message);
    g_error_free (error);
  }
}

// -------------------------------------------------------------------------------
gpointer Application::AnnoucementListener (Application *application)
{
  struct sockaddr_in addr;
  int                fd;

  if ((fd = socket (AF_INET,
                    SOCK_DGRAM,
                    0)) < 0)
  {
    g_warning ("socket: %s", strerror (errno));
    return NULL;
  }

  // allow multiple sockets to use the same PORT number
  {
    guint yes = 1;

    if (setsockopt (fd,
                    SOL_SOCKET,
                    SO_REUSEADDR,
                    &yes,
                    sizeof (yes)) < 0)
    {
      g_warning ("setsockopt: %s", strerror (errno));
      return NULL;
    }
  }

  {
    memset (&addr,
            0,
            sizeof(addr));

    addr.sin_family      = AF_INET;
    addr.sin_addr.s_addr = htonl (INADDR_ANY);
    addr.sin_port        = htons (ANNOUNCE_PORT);
  }

  if (bind (fd,
            (struct sockaddr *) &addr,
            sizeof (addr)) < 0)
  {
    g_warning ("socket: %s", strerror (errno));
    return NULL;
  }

  // use setsockopt() to request that the kernel join a multicast group
  {
    struct ip_mreq option;

    option.imr_multiaddr.s_addr = inet_addr (ANNOUNCE_GROUP);
    option.imr_interface.s_addr = htonl     (INADDR_ANY);
    if (setsockopt (fd,
                    IPPROTO_IP,
                    IP_ADD_MEMBERSHIP,
                    &option,
                    sizeof (option)) < 0)
    {
      g_warning ("setsockopt: %s", strerror (errno));
      return NULL;
    }
  }

  while (1)
  {
    struct sockaddr_in from;
    socklen_t          addrlen = sizeof (from);
    char               message[100];
    ssize_t            size;

    if ((size = recvfrom (fd,
                          message,
                          100,
                          0,
                          (struct sockaddr *) &from,
                          &addrlen)) < 0)
    {
      g_warning ("recvfrom: %s", strerror (errno));
    }

    application->OnNewPartner (new Partner (message,
                                            &from));
  }

  return NULL;
}

// --------------------------------------------------------------------------------
void Application::OnNewPartner (Partner *partner)
{
}

// --------------------------------------------------------------------------------
gboolean Application::OnLatestVersionReceived (Net::Downloader::CallbackData *cbk_data)
{
  Application *application = (Application *) cbk_data->_user_data;
  gchar       *data        = application->_version_downloader->GetData ();

  if (data)
  {
    GKeyFile *version_file = g_key_file_new ();

    if (g_key_file_load_from_data (version_file,
                                   data,
                                   strlen (data) + 1,
                                   G_KEY_FILE_NONE,
                                   NULL))
    {
      gboolean     new_version_detected = FALSE;
      const gchar *local_version        = VERSION;
      const gchar *local_revision       = VERSION_REVISION;
      const gchar *local_maturity       = VERSION_MATURITY;
      gchar       *remote_version;
      gchar       *remote_revision;
      gchar       *remote_maturity;

      remote_version = g_key_file_get_string (version_file,
                                              VERSION_BRANCH,
                                              "version",
                                              NULL);
      remote_revision = g_key_file_get_string (version_file,
                                               VERSION_BRANCH,
                                               "revision",
                                               NULL);
      remote_maturity = g_key_file_get_string (version_file,
                                               VERSION_BRANCH,
                                               "maturity",
                                               NULL);

      if (remote_version && remote_revision && remote_maturity)
      {
        if (atoi (local_version) < atoi (remote_version))
        {
          new_version_detected = TRUE;
        }
        else if (atoi (local_version) == atoi (remote_version))
        {
          if (strcmp (local_maturity, remote_maturity) != 0)
          {
            if (*remote_maturity == '\0')
            {
              new_version_detected = TRUE;
            }
            else if (strcmp (local_maturity, remote_maturity) < 0)
            {
              new_version_detected = TRUE;
            }
          }
          else if (atoi (local_revision) < atoi (remote_revision))
          {
            new_version_detected = TRUE;
          }
        }
        else if (strcmp (VERSION_BRANCH, "UNSTABLE") == 0)
        {
          char *stable_version = g_key_file_get_string (version_file,
                                                        "STABLE",
                                                        "version",
                                                        NULL);
          if (stable_version && strcmp (remote_version, stable_version) <= 0)
          {
            new_version_detected = TRUE;
          }
        }
      }

      if (new_version_detected)
      {
        Module *main_module = application->_main_module;
        gchar  *label       = g_strdup_printf ("%s.%s.%s", remote_version, remote_revision, remote_maturity);

        gtk_menu_item_set_label (GTK_MENU_ITEM (main_module->GetGObject ("new_version_menuitem")),
                                 label);
        gtk_widget_show (GTK_WIDGET (main_module->GetGObject ("update_menuitem")));
        g_free (label);
      }
    }

    g_key_file_free (version_file);
  }

  application->_version_downloader->Kill    ();
  application->_version_downloader->Release ();
  application->_version_downloader = NULL;

  return FALSE;
}

// --------------------------------------------------------------------------------
gboolean Application::OnHttpPost (const gchar *data)
{
  if (strcmp (data, "tagada soinsoin") == 0)
  {
    _granted = TRUE;
    return TRUE;
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
gchar *Application::OnHttpGet (const gchar *url)
{
  return NULL;
}

// --------------------------------------------------------------------------------
gchar *Application::GetSecretKey (const gchar *authentication_scheme)
{
  return NULL;
}

// --------------------------------------------------------------------------------
gboolean Application::HttpPostCbk (Net::HttpServer::Client *client,
                                   const gchar             *data)
{
  Application *app = (Application *) client;

  return app->OnHttpPost (data);
}

// --------------------------------------------------------------------------------
gchar *Application::HttpGetCbk (Net::HttpServer::Client *client,
                                const gchar             *url)
{
  Application *app = (Application *) client;

  return app->OnHttpGet (url);
}

// --------------------------------------------------------------------------------
void Application::AboutDialogActivateLinkFunc (GtkAboutDialog *about,
                                               const gchar    *link,
                                               gpointer        data)
{
#ifdef WINDOWS_TEMPORARY_PATCH
  ShellExecuteA (NULL,
                 "open",
                 link,
                 NULL,
                 NULL,
                 SW_SHOWNORMAL);
#else
  gtk_show_uri (NULL,
                link,
                GDK_CURRENT_TIME,
                NULL);
#endif
}

// --------------------------------------------------------------------------------
gint Application::CompareRanking (Attribute *attr_a,
                                  Attribute *attr_b)
{
  gint value_a = 0;
  gint value_b = 0;

  if (attr_a)
  {
    value_a = attr_a->GetIntValue ();
  }
  if (attr_b)
  {
    value_b = attr_b->GetIntValue ();
  }

  if (value_a == 0)
  {
    value_a = G_MAXINT;
  }
  if (value_b == 0)
  {
    value_b = G_MAXINT;
  }

  return (value_a - value_b);
}

// --------------------------------------------------------------------------------
gint Application::CompareDate (Attribute *attr_a,
                               Attribute *attr_b)
{
  GDate date_a;
  GDate date_b;

  if (attr_a)
  {
    g_date_set_parse (&date_a,
                      attr_a->GetStrValue ());
  }

  if (attr_b)
  {
    g_date_set_parse (&date_b,
                      attr_b->GetStrValue ());
  }

  return g_date_compare (&date_a,
                         &date_b);
}
// --------------------------------------------------------------------------------
#ifdef DEBUG
void Application::LogHandler (const gchar    *log_domain,
                              GLogLevelFlags  log_level,
                              const gchar    *message,
                              gpointer        user_data)
{
  switch (log_level)
  {
    case G_LOG_FLAG_FATAL:
    case G_LOG_LEVEL_ERROR:
    case G_LOG_LEVEL_CRITICAL:
    case G_LOG_LEVEL_WARNING:
    case G_LOG_FLAG_RECURSION:
    {
      g_print (RED "[%s]" ESC " %s\n", log_domain, message);
    }
    break;

    case G_LOG_LEVEL_MESSAGE:
    case G_LOG_LEVEL_INFO:
    case G_LOG_LEVEL_DEBUG:
    case G_LOG_LEVEL_MASK:
    default:
    {
      g_print (BLUE "[%s]" ESC " %s\n", log_domain, message);
    }
    break;
  }
}
#endif

// --------------------------------------------------------------------------------
gboolean Application::IsCsvReady (AttributeDesc *desc)
{
  return (   (desc->_scope       == AttributeDesc::GLOBAL)
          && (desc->_rights      == AttributeDesc::PUBLIC)
          && (desc->_persistency == AttributeDesc::PERSISTENT)
          && (g_ascii_strcasecmp (desc->_code_name, "final_rank") != 0)
          && (g_ascii_strcasecmp (desc->_code_name, "IP") != 0)
          && (g_ascii_strcasecmp (desc->_code_name, "exported")   != 0));
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_new_version_menuitem_activate (GtkMenuItem *menuitem,
                                                                  Object      *owner)
{
#ifdef WINDOWS_TEMPORARY_PATCH
  ShellExecuteA (NULL,
                 "open",
                 "http://betton.escrime.free.fr/index.php/bellepoule-telechargement",
                 NULL,
                 NULL,
                 SW_SHOWNORMAL);
#else
  gtk_show_uri (NULL,
                "http://betton.escrime.free.fr/index.php/bellepoule-telechargement",
                GDK_CURRENT_TIME,
                NULL);
#endif
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_translate_menuitem_activate (GtkWidget *w,
                                                                Object    *owner)
{
#ifdef WINDOWS_TEMPORARY_PATCH
  ShellExecuteA (NULL,
                 "open",
                 "http://betton.escrime.free.fr/index.php/developpement/translation-guidelines",
                 NULL,
                 NULL,
                 SW_SHOWNORMAL);
#else
  gtk_show_uri (NULL,
                "http://betton.escrime.free.fr/index.php/developpement/translation-guidelines",
                GDK_CURRENT_TIME,
                NULL);
#endif
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_about_menuitem_activate (GtkWidget *w,
                                                            Object    *owner)
{
  Module    *m      = dynamic_cast <Module *> (owner);
  GtkDialog *dialog = GTK_DIALOG (m->GetGObject ("about_dialog"));

  gtk_dialog_run  (dialog);
  gtk_widget_hide (GTK_WIDGET (dialog));
}
