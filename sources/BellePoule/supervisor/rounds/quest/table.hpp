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

#pragma once

#include <gtk/gtk.h>

#include "../tableau/table_supervisor.hpp"

namespace Generic
{
  class PointSystem;
}

class StageClass;

namespace Quest
{
  class BonusTable : public Table::Supervisor
  {
    public:
      static void Declare ();

      BonusTable (StageClass *stage_class);

    public:
      static const gchar *_class_name;
      static const gchar *_xml_class_name;

    private:
      ~BonusTable () override;

      Generic::PointSystem *GetPointSystem () override;

      GSList *SpreadAttendees (GSList *attendees) override;

      void Reset () override;

      const gchar *GetXmlClassName () override;

      static Stage *CreateInstance (StageClass *stage_class);
  };
}
