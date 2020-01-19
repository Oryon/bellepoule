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

#include <zip.h>

#include "util/global.hpp"
#include "util/player.hpp"
#include "util/attribute_desc.hpp"
#include "util/filter.hpp"
#include "rounds/barrage/barrage.hpp"
#include "rounds/checkin/checkin_supervisor.hpp"
#include "rounds/classification/general_classification.hpp"
#include "rounds/splitting/splitting.hpp"
#include "rounds/swiss/round.hpp"
#include "rounds/swiss/table.hpp"
#include "rounds/poule/pool_allocator.hpp"
#include "rounds/poule/pool_supervisor.hpp"
#include "rounds/tableau/table_supervisor.hpp"
#include "actors/fencer.hpp"
#include "actors/referee.hpp"
#include "actors/team.hpp"
#include "application/application.hpp"

#include "contest.hpp"
#include "tournament.hpp"

// --------------------------------------------------------------------------------
class BellPouleApp : public Application
{
  public:
    BellPouleApp (int *argc, char ***argv);

  private:
    Tournament *_tournament;

    ~BellPouleApp () override;

    void Prepare () override;

    void Start (int                   argc,
                char                **argv,
                Net::Ring::Listener  *ring_listener) override;

    void OnQuit (GtkWindow *window) override;
};

// --------------------------------------------------------------------------------
BellPouleApp::BellPouleApp (int    *argc,
                            char ***argv)
  : Application (Net::Ring::Role::RESOURCE_USER, "BellePoule", argc, argv)
{
  Global::_www = g_build_filename (Global::_share_dir, "webserver", "LightTPD", "www", "cotcot", NULL);

  if (g_file_test (Global::_www,
                   G_FILE_TEST_IS_DIR) == FALSE)
  {
    g_free (Global::_www);

    Global::_www = g_build_filename (g_get_user_config_dir (), "BellePoule", "www", "cotcot", NULL);

    {
      int         error_code = ZIP_ER_OK;
      struct zip *zip_archive;
      gchar      *zip_path    = g_build_filename (Global::_share_dir, "resources", "www.zip", NULL);

      zip_archive = zip_open (zip_path,
                              ZIP_CHECKCONS,
                              &error_code);

      if (error_code != ZIP_ER_OK)
      {
        zip_error_t zip_error;

        zip_error_init_with_code (&zip_error,
                                  error_code);

        g_warning ("zip_open (%s): %s\n", zip_path, zip_error_strerror (&zip_error));
        zip_error_fini (&zip_error);
      }
      else
      {
        guint count = zip_get_num_files (zip_archive);

        for (guint i = 0; i <= count; i++)
        {
          struct zip_stat  file_stat;

          zip_stat_index (zip_archive,
                          i,
                          0,
                          &file_stat);

          if (zip_stat (zip_archive,
                        file_stat.name,
                        0,
                        &file_stat) == -1)
          {
            g_warning ("zip_stat: %s\n", zip_strerror (zip_archive));
          }
          else
          {
            struct zip_file *file_zip = zip_fopen (zip_archive,
                                                   file_stat.name,
                                                   ZIP_FL_UNCHANGED);

            if (file_zip == nullptr)
            {
              g_warning ("zip_fopen: %s\n", zip_strerror (zip_archive));
            }
            else
            {
              gchar *content = g_new0 (gchar,
                                       file_stat.size+1);

              if (zip_fread (file_zip,
                             content,
                             file_stat.size) != (zip_int64_t) file_stat.size)
              {
                g_warning ("zip_fread: %s\n", zip_strerror (zip_archive));
              }
              else
              {
                GError *error    = nullptr;
                gchar  *out_file = g_build_filename (Global::_www, "..", "..", file_stat.name, NULL);
                gchar  *dirname  = g_path_get_dirname (out_file);

                g_mkdir_with_parents (dirname,
                                      0755);

                if (g_file_set_contents (out_file,
                                         content,
                                         file_stat.size,
                                         &error) == FALSE)
                {
                  g_warning ("g_file_set_contents: %s", error->message);
                  g_error_free (error);
                }

                g_free (dirname);
                g_free (out_file);
              }

              zip_fclose (file_zip);
              g_free (content);
            }
          }
        }

        zip_close (zip_archive);
      }
      g_free (zip_path);
    }
  }
}

// --------------------------------------------------------------------------------
BellPouleApp::~BellPouleApp ()
{
  _tournament->Release ();
}

// --------------------------------------------------------------------------------
void BellPouleApp::Prepare ()
{
  Tournament::Init ();
  Contest::Init    ();

  {
    People::CheckinSupervisor::Declare     ();
    Pool::Allocator::Declare               ();
    Pool::Supervisor::Declare              ();
    Swiss::Round::Declare                  ();
    Swiss::Toto::Declare                   ();
    Table::Supervisor::Declare             ();
    People::Barrage::Declare               ();
    People::GeneralClassification::Declare ();
    People::Splitting::Declare             ();
  }

  {
    Fencer::RegisterPlayerClass  ();
    Team::RegisterPlayerClass    ();
    Referee::RegisterPlayerClass ();
  }

  Application::Prepare ();
}

// --------------------------------------------------------------------------------
void BellPouleApp::Start (int                   argc,
                          char                **argv,
                          Net::Ring::Listener  *ring_listener)
{
  _tournament = new Tournament ();

  People::Splitting::SetHostTournament (_tournament);

  _main_module = _tournament;
  _main_module->SetData (nullptr,
                         "application",
                         this);
  Application::Start (argc,
                      argv,
                      _tournament);

  if (argc > 1)
  {
    _tournament->Start (g_strdup (argv[1]));
  }
  else
  {
    _tournament->Start (nullptr);
  }

  gtk_main ();

  Object::DumpList ();
}

// --------------------------------------------------------------------------------
void BellPouleApp::OnQuit (GtkWindow *window)
{
  GtkWidget *dialog = gtk_message_dialog_new_with_markup (window,
                                                          GTK_DIALOG_DESTROY_WITH_PARENT,
                                                          GTK_MESSAGE_QUESTION,
                                                          GTK_BUTTONS_OK_CANCEL,
                                                          gettext ("<b><big>Do you really want to quit BellePoule</big></b>"));

  gtk_window_set_title (GTK_WINDOW (dialog),
                        gettext ("Quit BellePoule?"));

  gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
                                            gettext ("All the unsaved competions will be lost."));

  if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_OK)
  {
    Application::OnQuit (window);
  }

  gtk_widget_destroy (dialog);
}

// --------------------------------------------------------------------------------
int main (int argc, char **argv)
{
  Application *application = new BellPouleApp (&argc, &argv);

  application->Prepare ();

  application->Start (argc,
                      argv,
                      nullptr);

  return 0;
}
