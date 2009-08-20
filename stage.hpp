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

#ifndef stage_hpp
#define stage_hpp

#include <libxml/xmlwriter.h>
#include <gtk/gtk.h>

#include "object.hpp"

class Stage_c : public virtual Object_c
{
  public:
    void SetPrevious (Stage_c *previous);

    Stage_c *GetPreviousStage ();

    gchar *GetName ();

    gboolean Locked ();

    void Lock ();

    void UnLock ();

    virtual void Enter ();

    virtual void Wipe ();

    virtual void Load (xmlDoc *doc);

    virtual void Save (xmlTextWriter *xml_writer);

  protected:
    Stage_c (gchar *name);

    virtual ~Stage_c ();

  private:
    Stage_c  *_previous;
    gchar    *_name;
    gboolean  _locked;

    virtual void OnLocked ();
    virtual void OnUnLocked ();
};

#endif
