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

#include <locale.h>

#include "util/global.hpp"
#include "util/user_config.hpp"

#include "language.hpp"

#ifdef OSX
// --------------------------------------------------------------------------------
static gchar *GetRelocatedLocale (const gchar *term,
                                  const gchar *relocated_domain)
{
  return (gchar *) g_dpgettext2 (relocated_domain, "Stock label", term);
}
#endif

// --------------------------------------------------------------------------------
Language::Language ()
  : Object ("Language")
{
  // Activate the language selected
  {
    gchar *user_language = g_key_file_get_string (Global::_user_config->_key_file,
                                                  "Tournament",
                                                  "interface_language",
                                                  nullptr);

    if (user_language == nullptr)
    {
      user_language = g_ascii_strdown (setlocale (LC_MESSAGES, nullptr), -1);

      if (user_language)
      {
        char *separator = strchr (user_language, '_');

        if (separator)
        {
          *separator = '\0';
        }
      }
    }

    if (user_language)
    {
      g_setenv ("LANGUAGE",
                user_language,
                TRUE);
      g_free (user_language);
    }
  }

  // Countries
  {
    gchar *translation_path = g_build_filename (Global::_share_dir, "resources", "countries", "translations", NULL);

    bindtextdomain ("countries", translation_path);
    bind_textdomain_codeset ("countries", "UTF-8");

    g_free (translation_path);
  }

  // Items
  {
    gchar *translation_path = g_build_filename (Global::_share_dir, "resources", "translations", NULL);

    bindtextdomain ("BellePoule", translation_path);
    bind_textdomain_codeset ("BellePoule", "UTF-8");

    g_free (translation_path);
  }

#ifdef OSX
  {
    gchar *gtk_locale_path = g_build_filename (Global::_share_dir, "..", "locale", NULL);

    bindtextdomain ("gtk20r", gtk_locale_path);
    bind_textdomain_codeset ("gtk20r", "UTF-8");

    g_free (gtk_locale_path);
  }

  {
    GtkStockItem item;

    // Force the creation of the stock hash table (workaround)
    gtk_stock_lookup (GTK_STOCK_ABOUT,
                      &item);

    gtk_stock_set_translate_func ("gtk20",
                                  (GtkTranslateFunc) GetRelocatedLocale,
                                  (gpointer) "gtk20r",
                                  NULL);
  }

  {
    gchar        *gtk_icons_path = g_build_filename (Global::_share_dir, "..", "icons", NULL);
    GtkIconTheme *icon_theme     = gtk_icon_theme_get_default ();

    gtk_icon_theme_prepend_search_path (icon_theme,
                                        gtk_icons_path);

    g_free (gtk_icons_path);
  }
#endif

  textdomain ("BellePoule");
}

// --------------------------------------------------------------------------------
Language::~Language ()
{
}

// --------------------------------------------------------------------------------
void Language::OnLocaleToggled (GtkCheckMenuItem *checkmenuitem,
                                gchar            *locale)
{
  if (gtk_check_menu_item_get_active (checkmenuitem))
  {
    g_key_file_set_string (Global::_user_config->_key_file,
                           "Tournament",
                           "interface_language",
                           locale);
  }
}

// --------------------------------------------------------------------------------
void Language::Populate (GtkMenuItem  *menu_item,
                         GtkMenuShell *menu_shell)
{
  gchar  *contents;
  gchar  *filename = g_build_filename (Global::_share_dir, "resources", "translations", "index.txt", NULL);

  if (g_file_get_contents (filename,
                           &contents,
                           nullptr,
                           nullptr) == TRUE)
  {
    gchar **lines = g_strsplit_set (contents, "\n", 0);

    if (lines)
    {
      gchar  *favorite = g_strdup (g_getenv ("LANGUAGE"));
      GSList *group    = nullptr;

      for (guint l = 0; lines[l] && lines[l][0]; l++)
      {
        gchar **tokens = g_strsplit_set (lines[l], ";", 0);

        if (tokens)
        {
          GtkWidget *item;

          g_setenv ("LANGUAGE",
                    tokens[0],
                    TRUE);
          textdomain ("BellePoule");

          g_strdelimit (tokens[1],
                        "\n\r",
                        '\0');

          item  = gtk_radio_menu_item_new_with_label (group, tokens[1]);
          group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (item));

          gtk_menu_shell_append (menu_shell,
                                 item);
          gtk_widget_set_tooltip_text (item,
                                       gettext ("Restart BellePoule for this change to take effect"));

          g_signal_connect (item, "toggled",
                            G_CALLBACK (OnLocaleToggled), (void *)
                            g_strdup (tokens[0]));
          if (g_strcmp0 (favorite, tokens[0]) == 0)
          {
            gtk_menu_item_set_label (menu_item,
                                     tokens[0]);
            gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (item),
                                            TRUE);
          }

          gtk_widget_show (item);
          g_strfreev (tokens);
        }
      }

      if (favorite)
      {
        gtk_menu_item_set_label (menu_item,
                                 favorite);
        g_setenv ("LANGUAGE",
                  favorite,
                  TRUE);
        textdomain ("BellePoule");
        g_free (favorite);
      }
      else
      {
        g_unsetenv ("LANGUAGE");
      }

      g_strfreev (lines);
    }
    g_free (contents);
  }

  g_free (filename);
}
