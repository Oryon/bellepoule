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
#include <sys/types.h>
#include <gdk/gdkkeysyms.h>

#ifdef WINDOWS_TEMPORARY_PATCH
#  ifndef WIN32_LEAN_AND_MEAN
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <windows.h>
#  include <shellapi.h>
#endif

#ifdef OSX
#include <execinfo.h>
#include "CoreFoundation/CoreFoundation.h"
#endif

#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include "locale"

#include "util/user_config.hpp"
#include "util/attribute.hpp"
#include "util/global.hpp"
#include "util/module.hpp"
#include "network/ring.hpp"
#include "network/message.hpp"
#include "network/downloader.hpp"
#include "language.hpp"
#include "weapon.hpp"
#include "version.h"

#include "application.hpp"

// --------------------------------------------------------------------------------
Application::Application (Net::Ring::Role     role,
                          const gchar        *public_name,
                          int                *argc,
                          char             ***argv)
  : Object ("Application")
{
  _language    = nullptr;
  _main_module = nullptr;

  g_setenv ("SUDO_ASKPASS",
            "/usr/bin/ssh-askpass",
            FALSE);

  g_setenv ("UBUNTU_MENUPROXY",
            "0",
            TRUE);

  g_unsetenv ("http_proxy");
  g_unsetenv ("https_proxy");

  _role = role;

  // g_mem_set_vtable (glib_mem_profiler_table);

  // Init
  {
    {
      gchar *binary_dir;

#ifndef OSX
      {
        gchar *program = g_find_program_in_path ((*argv)[0]);

        binary_dir = g_path_get_dirname (program);
        g_free (program);
      }
#else
      {
        //CFURLRef    absolute_url = CFURLCopyAbsoluteURL (url);
        CFBundleRef  bundle      = CFBundleGetMainBundle ();
        CFURLRef     url         = CFBundleCopyBundleURL (bundle);
        CFStringRef  string_path = CFURLCopyFileSystemPath (url, kCFURLPOSIXPathStyle);
        gchar       *bundle_dir;

        {
          CFIndex len = CFStringGetMaximumSizeForEncoding (CFStringGetLength (string_path),
                                                           kCFStringEncodingUTF8) + 1;

          bundle_dir = g_new0 (gchar, len);

          CFStringGetCString (string_path,
                              bundle_dir,
                              len,
                              kCFStringEncodingUTF8);
        }

        binary_dir = g_build_filename (bundle_dir, "Contents", "MacOS", NULL);

        g_free (bundle_dir);
        CFRelease (string_path);
        CFRelease (url);
      }
#endif

#if defined(DEBUG)
      g_log_set_default_handler (LogHandler,
                                 nullptr);
#endif

#ifdef CODE_BLOCKS
      Global::_share_dir = g_build_filename (binary_dir, "..", "..", "..", NULL);
#else
      {
        gchar  *basename = g_path_get_basename ((*argv)[0]);
        gchar **segments = g_strsplit_set (basename,
                                           "-.",
                                           0);

        if (segments[0])
        {
#ifdef OSX
          gchar *root_dir = g_build_filename (binary_dir, "..", "Resources", NULL);

          Global::_share_dir = g_build_filename (binary_dir, root_dir, "share", segments[0], NULL);

          {
            gchar *pixbuf_loaders = g_build_filename (root_dir, "etc", "gtk-2.0", "gdk-pixbuf.loaders", NULL);
            gchar *xdg_path       = g_build_filename (root_dir, "share", NULL);

            g_setenv ("GDK_PIXBUF_MODULE_FILE",
                      pixbuf_loaders,
                      TRUE);

            g_setenv ("XDG_DATA_DIRS",
                      xdg_path,
                      TRUE);

            g_chdir (binary_dir);

            g_free (pixbuf_loaders);
            g_free (xdg_path);
          }

          g_setenv ("GTK_PATH",
                    root_dir,
                    TRUE);
          g_setenv ("GTK_EXE_PREFIX",
                    root_dir,
                    TRUE);

          {
            CFLocaleRef  locale_ref = CFLocaleCopyCurrent ();
            CFStringRef  locale_str = CFLocaleGetIdentifier (locale_ref);
            CFIndex      len;
            gchar       *locale_utf8;

            len = CFStringGetMaximumSizeForEncoding (CFStringGetLength (locale_str),
                                                     kCFStringEncodingUTF8) + 1;

            locale_utf8 = g_new0 (gchar, len);

            if (CFStringGetCString (locale_str,
                                    locale_utf8,
                                    len,
                                    kCFStringEncodingUTF8))
            {
              setlocale (LC_MESSAGES, locale_utf8);
            }

            g_free (locale_utf8);
            CFRelease (locale_str);
            CFRelease (locale_ref);
          }
          g_free (root_dir);
#else
          Global::_share_dir = g_build_filename (binary_dir, "..", "share", segments[0], NULL);
#endif
        }

        g_strfreev (segments);
        g_free     (basename);
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
      g_object_set (gtk_settings_get_default (), "gtk-menu-images",   TRUE, NULL);
      g_object_set (gtk_settings_get_default (), "gtk-button-images", TRUE, NULL);
    }

    //Object::Track ("Player");
  }

#if GTK_MAJOR_VERSION < 3
  gtk_about_dialog_set_url_hook (AboutDialogActivateLinkFunc,
                                 NULL,
                                 NULL);
#endif

  curl_global_init (CURL_GLOBAL_ALL);

  Global::_user_config = new UserConfig (public_name);

  {
    _version_downloader = new Net::Downloader ("_version_downloader",
                                               this);
    _version_downloader->Start ("http://betton.escrime.free.fr/documents/BellePoule/latest.html");
  }
}

// --------------------------------------------------------------------------------
Application::~Application ()
{
  AttributeDesc::Cleanup ();

  _language->Release ();

  if (_version_downloader)
  {
    _version_downloader->Kill    ();
    _version_downloader->Release ();
  }

  Weapon::FreeList ();

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
    new Weapon ("Educ",  "X", "S", 30);
    new Weapon ("Foil",  "F", "F", 30);
    new Weapon ("Epée",  "E", "E", 30);
    new Weapon ("Sabre", "S", "S", 20);
  }

  // Attributes definition
  {
    AttributeDesc *desc;

    desc = AttributeDesc::Declare (G_TYPE_UINT, "ref", "ID", (gchar *) "ref");
    desc->_rights = AttributeDesc::PRIVATE;

    desc = AttributeDesc::Declare (G_TYPE_UINT, "plugin_ID", "PluginID", (gchar *) "PluginID");
    desc->_rights = AttributeDesc::PRIVATE;

    desc = AttributeDesc::Declare (G_TYPE_UINT, "final_rank", "Classement", gettext ("place"));

    desc = AttributeDesc::Declare (G_TYPE_STRING, "team", "Equipe", gettext ("team"));
    desc->_uniqueness = AttributeDesc::NOT_SINGULAR;
    desc->AddDiscreteValues (nullptr, nullptr, nullptr, NULL);
    desc->EnableSorting ();

    desc = AttributeDesc::Declare (G_TYPE_STRING, "name", "Nom", gettext ("last name"));
    desc->_short_length = 15;

    desc = AttributeDesc::Declare (G_TYPE_STRING, "first_name", "Prenom", gettext ("first name"));

    desc = AttributeDesc::Declare (G_TYPE_STRING, "birth_date", "DateNaissance", gettext ("date of birth"));
    desc->_compare_func = (GCompareFunc) CompareDate;

    desc = AttributeDesc::Declare (G_TYPE_STRING, "gender", "Sexe", gettext ("gender"));
    desc->_uniqueness         = AttributeDesc::NOT_SINGULAR;
    desc->_free_value_allowed = FALSE;
    desc->_favorite_look      = AttributeDesc::GRAPHICAL;
    desc->AddDiscreteValues ("M", gettext ("Male"), "resources/glade/images/male.png",
                             "F", gettext ("Female"), "resources/glade/images/female.png", NULL);

    desc = AttributeDesc::Declare (G_TYPE_STRING, "weapon", "Arme", gettext ("weapon"));
    desc->_uniqueness         = AttributeDesc::NOT_SINGULAR;
    desc->_rights             = AttributeDesc::PRIVATE;
    desc->_free_value_allowed = FALSE;
    desc->_favorite_look      = AttributeDesc::SHORT_TEXT;
    desc->AddDiscreteValues ("X", gettext ("Educ"),  nullptr,
                             "F", gettext ("Foil"),  NULL,
                             "E", gettext ("Epée"),  NULL,
                             "S", gettext ("Sabre"), NULL, NULL);

    desc = AttributeDesc::Declare (G_TYPE_STRING, "country", "Nation", gettext ("country"));
    desc->_uniqueness = AttributeDesc::NOT_SINGULAR;
    desc->_favorite_look = AttributeDesc::GRAPHICAL;
    desc->AddDiscreteValueSelector ("countries");
    AttributeDesc::AddSwappable (desc);

    desc = AttributeDesc::Declare (G_TYPE_STRING, "region", "Region", gettext ("region"));
    desc->_favorite_look = AttributeDesc::GRAPHICAL;
    desc->_uniqueness = AttributeDesc::NOT_SINGULAR;
    desc->AddLocalizedDiscreteValues ("regions");
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

    desc = AttributeDesc::Declare (G_TYPE_UINT, "workload_rate", "Activite", gettext ("rate"));
    desc->_persistency    = AttributeDesc::NOT_PERSISTENT;
    desc->_favorite_look  = AttributeDesc::GRAPHICAL;
    desc->_rights         = AttributeDesc::PRIVATE;

    desc = AttributeDesc::Declare (G_TYPE_STRING, "category", "Categorie", gettext ("level"));
    desc->_uniqueness = AttributeDesc::NOT_SINGULAR;

    desc = AttributeDesc::Declare (G_TYPE_UINT, "ranking", "Ranking", gettext ("ranking"));
    desc->_compare_func = (GCompareFunc) CompareRanking;

    desc = AttributeDesc::Declare (G_TYPE_STRING, "rating", "Rating", gettext ("rating"));

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
    desc->AddDiscreteValues ("Q", gettext ("Qualified"),     "resources/glade/images/normal.png",
                             "N", gettext ("Not qualified"), "resources/glade/images/exit.png",
                             "A", gettext ("Withdrawal"),    "resources/glade/images/ambulance.png",
                             "E", gettext ("Excluded"),      "resources/glade/images/black_card.png",
                             "F", gettext ("Forfeit"),       "resources/glade/images/normal.png", NULL);

    desc = AttributeDesc::Declare (G_TYPE_STRING, "status", "Statut", gettext ("status"));
    desc->_scope = AttributeDesc::LOCAL;
    desc->_favorite_look = AttributeDesc::GRAPHICAL;
    desc->AddDiscreteValues ("Q", gettext ("Qualified"),     "resources/glade/images/normal.png",
                             "N", gettext ("Not qualified"), "resources/glade/images/exit.png",
                             "A", gettext ("Withdrawal"),    "resources/glade/images/ambulance.png",
                             "E", gettext ("Excluded"),      "resources/glade/images/black_card.png",
                             "F", gettext ("Forfeit"),       "resources/glade/images/normal.png", NULL);

    desc = AttributeDesc::Declare (G_TYPE_STRING, "custom1", "Divers1", (gchar *) "♥");
    desc->_uniqueness = AttributeDesc::NOT_SINGULAR;
    desc = AttributeDesc::Declare (G_TYPE_STRING, "custom2", "Divers2", (gchar *) "♣");
    desc->_uniqueness = AttributeDesc::NOT_SINGULAR;
    desc = AttributeDesc::Declare (G_TYPE_STRING, "custom3", "Divers3", (gchar *) "♠");
    desc->_uniqueness = AttributeDesc::NOT_SINGULAR;
    desc = AttributeDesc::Declare (G_TYPE_STRING, "custom4", "Divers4", (gchar *) "♦");
    desc->_uniqueness = AttributeDesc::NOT_SINGULAR;

    desc = AttributeDesc::Declare (G_TYPE_STRING, "cyphered_password", "MotDePasseChiffre", gettext ("cyphered password"));
    desc->_rights = AttributeDesc::PRIVATE;

    // Not persistent data
    {
      desc = AttributeDesc::Declare (G_TYPE_STRING, "IP", "IP", gettext ("IP address"));
      desc->_persistency = AttributeDesc::NOT_PERSISTENT;
      desc->_rights      = AttributeDesc::PRIVATE;

      desc = AttributeDesc::Declare (G_TYPE_STRING, "password", "MotDePasse", gettext ("Password"));
      desc->_persistency = AttributeDesc::NOT_PERSISTENT;
      desc->_rights      = AttributeDesc::PRIVATE;

      desc = AttributeDesc::Declare (G_TYPE_UINT, "pool_nr", "PoolID", gettext ("pool #"));
      desc->_persistency = AttributeDesc::NOT_PERSISTENT;
      desc->_scope       = AttributeDesc::LOCAL;

      desc = AttributeDesc::Declare (G_TYPE_UINT, "victories_count", "Victoires", gettext ("Victories"));
      desc->_persistency = AttributeDesc::NOT_PERSISTENT;
      desc->_scope       = AttributeDesc::LOCAL;

      desc = AttributeDesc::Declare (G_TYPE_UINT, "bouts_count", "Assauts", gettext ("Bouts"));
      desc->_persistency = AttributeDesc::NOT_PERSISTENT;
      desc->_scope       = AttributeDesc::LOCAL;

      desc = AttributeDesc::Declare (G_TYPE_UINT, "victories_ratio", "RatioVictoires", gettext ("Vict./Bouts (‰)"));
      desc->_persistency = AttributeDesc::NOT_PERSISTENT;
      desc->_scope       = AttributeDesc::LOCAL;

      desc = AttributeDesc::Declare (G_TYPE_INT, "indice", "Indice", gettext ("index"));
      desc->_persistency = AttributeDesc::NOT_PERSISTENT;
      desc->_scope       = AttributeDesc::LOCAL;

      desc = AttributeDesc::Declare (G_TYPE_UINT, "HS", "TD", gettext ("Hits scored"));
      desc->_persistency = AttributeDesc::NOT_PERSISTENT;
      desc->_scope       = AttributeDesc::LOCAL;

      desc = AttributeDesc::Declare (G_TYPE_UINT, "rank", "rank", gettext ("place"));
      desc->_persistency = AttributeDesc::NOT_PERSISTENT;
      desc->_rights      = AttributeDesc::PRIVATE;
      desc->_scope       = AttributeDesc::LOCAL;

      desc = AttributeDesc::Declare (G_TYPE_UINT, "stage_start_rank", "RangDeDepart", gettext ("Round start rank"));
      desc->_persistency = AttributeDesc::NOT_PERSISTENT;
      desc->_rights      = AttributeDesc::PRIVATE;
      desc->_scope       = AttributeDesc::LOCAL;

      desc = AttributeDesc::Declare (G_TYPE_BOOLEAN, "promoted", "Promu", gettext ("promoted"));
      desc->_persistency   = AttributeDesc::NOT_PERSISTENT;
      desc->_scope         = AttributeDesc::LOCAL;
      desc->_favorite_look = AttributeDesc::GRAPHICAL;

      desc = AttributeDesc::Declare (G_TYPE_STRING, "connection", "Connexion", gettext ("connection"));
      desc->_persistency    = AttributeDesc::NOT_PERSISTENT;
      desc->_rights         = AttributeDesc::PRIVATE;
      desc->_scope          = AttributeDesc::GLOBAL;
      desc->_favorite_look  = AttributeDesc::GRAPHICAL;
      desc->AddDiscreteValues ("Broken",  gettext ("Broken"),  "resources/glade/images/red.png",
                               "Waiting", gettext ("Waiting"), "resources/glade/images/orange.png",
                               "OK",      gettext ("OK"),      "resources/glade/images/green.png",
                               "Manual",  gettext ("Manual"),  (gchar *) GTK_STOCK_EDIT, NULL);

    }

    AttributeDesc::SetCriteria ("CSV ready",
                                IsCsvReady);
  }
}

// --------------------------------------------------------------------------------
void Application::Start (int                   argc,
                         char                **argv,
                         Net::Ring::Listener  *ring_listener)
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
    GtkWidget *w           = GTK_WIDGET (_main_module->GetGObject ("about_dialog"));
    gchar     *translators = g_strdup_printf ("Julien Diaz           (German)\n"
                                              "Tina Schliemann       (German)\n"
                                              "Michael Weber         (German)\n"
                                              "Franz Lewin Wagner    (German)\n"
                                              "Dom Walden            (English\n"
                                              "Aureliano Martini     (Italian)\n"
                                              "Jihwan Cho            (Korean)\n"
                                              "Marijn Somers         (Dutch)\n"
                                              "Werner Huysmans       (Dutch)\n"
                                              "Farid Elkhalki        (Japanese)\n"
                                              "Jean-François Brun    (Japanese)\n"
                                              "Adolpho Jayme         (Spanish)\n"
                                              "Angel Oliver          (Spanish, Catalan)\n"
                                              "Alexis Pigeon         (Spanish)\n"
                                              "Paulo Morales         (Spanish)\n"
                                              "Ignacio Gil           (Spanish)\n"
                                              "Stefano Schirone      (Spanish)\n"
                                              "Dominik Denkiewicz    (Polish)\n"
                                              "Eduardo Alberto Calvo (Spanish, Portuguese)\n"
                                              "Ricardo Catanho       (Portuguese)\n"
                                              "Rui Pedro Vieira      (Portuguese)\n"
                                              "Mohamed Rebai         (Arabic)\n"
                                              "Mikael Hiort af Ornäs (Swedish)\n"
                                              "Sergev Makhtanov      (Russian)"
                                              "Alcedo Attis          (Hungarian)\n"
                                              "Simon Dobelsek        (Slovenian)\n"
                                              "Ibon Santisteban      (Basque)\n"
                                              "Ben Haeberli          (German)");

    gtk_about_dialog_set_translator_credits (GTK_ABOUT_DIALOG (w),
                                             translators);
    g_free (translators);
  }

  {
    GtkWidget *w = _main_module->GetRootWidget ();

    gtk_widget_show_all (w);
  }

  gtk_widget_hide (GTK_WIDGET (_main_module->GetGObject ("update_menuitem")));

  Net::Ring::Join (_role,
                   ring_listener,
                   GTK_WIDGET (_main_module->GetGObject ("ring_menuitem")));
}

// --------------------------------------------------------------------------------
void Application::OnDownloaderData (Net::Downloader  *downloader,
                                    const gchar      *data)
{
  if (data)
  {
    GKeyFile *version_file = g_key_file_new ();

    if (g_key_file_load_from_data (version_file,
                                   data,
                                   strlen (data) + 1,
                                   G_KEY_FILE_NONE,
                                   nullptr))
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
                                              nullptr);
      remote_revision = g_key_file_get_string (version_file,
                                               VERSION_BRANCH,
                                               "revision",
                                               nullptr);
      remote_maturity = g_key_file_get_string (version_file,
                                               VERSION_BRANCH,
                                               "maturity",
                                               nullptr);

      if (remote_version && remote_revision && remote_maturity)
      {
        if (atoi (local_version) < atoi (remote_version))
        {
          new_version_detected = TRUE;
        }
        else if (atoi (local_version) == atoi (remote_version))
        {
          if (g_strcmp0 (local_maturity, remote_maturity) != 0)
          {
            if (*remote_maturity == '\0')
            {
              new_version_detected = TRUE;
            }
            else if (g_strcmp0 (local_maturity, remote_maturity) < 0)
            {
              new_version_detected = TRUE;
            }
          }
          else if (atoi (local_revision) < atoi (remote_revision))
          {
            new_version_detected = TRUE;
          }
        }
        else if (g_strcmp0 (VERSION_BRANCH, "UNSTABLE") == 0)
        {
          char *stable_version = g_key_file_get_string (version_file,
                                                        "STABLE",
                                                        "version",
                                                        nullptr);
          if (stable_version && g_strcmp0 (remote_version, stable_version) <= 0)
          {
            new_version_detected = TRUE;
          }
        }
      }

      if (new_version_detected)
      {
        Module *main_module = _main_module;
        gchar  *label       = g_strdup_printf ("%s.%s.%s", remote_version, remote_revision, remote_maturity);

        gtk_menu_item_set_label (GTK_MENU_ITEM (main_module->GetGObject ("new_version_menuitem")),
                                 label);
        gtk_widget_show (GTK_WIDGET (main_module->GetGObject ("update_menuitem")));
        g_free (label);
      }
    }

    g_key_file_free (version_file);
  }

  _version_downloader->Release ();
  _version_downloader = nullptr;
}

// --------------------------------------------------------------------------------
const gchar *Application::GetSecretKey (const gchar *authentication_scheme)
{
  return nullptr;
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
  gtk_show_uri (nullptr,
                link,
                GDK_CURRENT_TIME,
                nullptr);
#endif
}

// --------------------------------------------------------------------------------
void Application::OnOpenUserManual ()
{
  gchar *uri           = nullptr;
  gchar *language_code = g_key_file_get_string (Global::_user_config->_key_file,
                                                "Tournament",
                                                "interface_language",
                                                nullptr);

  if (language_code)
  {
    uri = g_build_filename (Global::_share_dir, "resources", "translations", language_code, "user_manual.pdf", NULL);

    if (g_file_test (uri,
                     G_FILE_TEST_EXISTS) == FALSE)
    {
      g_free (uri);
      uri = nullptr;
    }

    g_free (language_code);
  }

  if (uri == nullptr)
  {
    uri = g_build_filename (Global::_share_dir, "resources", "translations", "user_manual.pdf", NULL);
  }


#ifdef WINDOWS_TEMPORARY_PATCH
  ShellExecute (NULL,
                "open",
                uri,
                NULL,
                NULL,
                SW_SHOWNORMAL);
#else
  {
    gchar *full_uri = g_build_filename ("file://", uri, NULL);

    gtk_show_uri (nullptr,
                  full_uri,
                  GDK_CURRENT_TIME,
                  nullptr);
    g_free (full_uri);
  }
#endif

  g_free (uri);
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

  if (   g_date_valid (&date_a)
      && g_date_valid (&date_b))
  {
    return g_date_compare (&date_a,
                           &date_b);
  }

  return attr_a->CompareWith (attr_b);
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
#ifdef OSX
  void  *callstack[128];
  int    frames         = backtrace(callstack, 128);
  char **strs           = backtrace_symbols(callstack, frames);

  for (int i = 0; i < frames; ++i)
  {
    printf("callstack: %s\n", strs[i]);
  }
  free (strs);
#endif
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
void Application::OnQuit (GtkWindow *window)
{
  Release ();
  Net::Ring::_broker->Leave ();
  gtk_widget_hide (GTK_WIDGET (window));
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
  gtk_show_uri (nullptr,
                "http://betton.escrime.free.fr/index.php/bellepoule-telechargement",
                GDK_CURRENT_TIME,
                nullptr);
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
  gtk_show_uri (nullptr,
                "http://betton.escrime.free.fr/index.php/developpement/translation-guidelines",
                GDK_CURRENT_TIME,
                nullptr);
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

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_user_manual_activate (GtkWidget *w,
                                                         Object    *owner)
{
  Application *a = (Application *) owner->GetPtrData (nullptr,
                                                      "application");

  a->OnOpenUserManual ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT gboolean on_root_key_press_event (GtkWidget   *widget,
                                                             GdkEventKey *event,
                                                             Object      *owner)
{
#ifdef DEBUG
  if (event->keyval == GDK_KEY_F11)
  {
    Object::DumpList ();
  }
#endif

  return FALSE;
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT gboolean on_root_delete_event (GtkWidget *w,
                                                          GdkEvent  *event,
                                                          Object    *owner)
{
  Application *a = (Application *) owner->GetPtrData (nullptr,
                                                      "application");

  a->OnQuit (GTK_WINDOW (gtk_widget_get_toplevel (w)));

  return TRUE;
}
