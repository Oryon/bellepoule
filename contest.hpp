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

#ifndef contest_hpp
#define contest_hpp

#include <libxml/xmlwriter.h>
#include <gtk/gtk.h>

#include "module.hpp"
#include "glade.hpp"
#include "players_list.hpp"
#include "players_base.hpp"
#include "schedule.hpp"
#include "pool.hpp"
#include "pool_supervisor.hpp"
#include "pools_list.hpp"

class Contest_c : public Module_c
{
  public:
     Contest_c (gchar *filename);
    ~Contest_c ();

    static void Init ();
    static Contest_c *Create ();

    void AttachTo (GtkNotebook *to);
    void Save     ();

  public:
    void on_previous_stage_toolbutton_clicked ();
    void on_next_stage_toolbutton_clicked     ();
    void on_save_toolbutton_clicked           ();
    void on_properties_toolbutton_clicked     ();
    void on_formula_toolbutton_clicked        ();
    void on_backupfile_button_clicked         ();
    void on_contest_close_button_clicked      ();

  private:
    static gchar *_NEW_CONTEST;

    gchar            *_name;
    gchar            *_filename;
    gchar            *_backup;
    PlayersBase_c    *_players_base;
    Schedule_c       *_schedule;
    GtkWidget        *_properties_dlg;
    GtkWidget        *_formula_dlg;
    PlayersList_c    *_player_list;
    PoolsList_c      *_pools_list;
    PoolSupervisor_c *_pool_supervisor;

    Contest_c ();

    void   InitInstance      ();
    void   ReadProperties    ();
    void   SetName           (gchar *name);
    gchar *GetSaveFileName   (gchar *title);
    void   Save              (gchar *filename);
};

#endif
