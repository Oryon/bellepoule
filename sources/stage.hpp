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
#include "sensitivity_trigger.hpp"

class Player;
class Classification;
class Filter;
class Contest;

class Stage : public virtual Object
{
  public:
    struct StageClass;

    typedef Stage * (*Creator) (StageClass *stage_class);

    typedef void (*StatusCbk) (Stage *stage,
                               void  *data);

    typedef enum
    {
      EDITABLE  = 0x0001
    } Rights;

    typedef enum
    {
      USER_ACTION,
      LOADING
    } Reason;

    struct StageClass
    {
      const gchar *_name;
      const gchar *_xml_name;
      Creator      _creator;
      Rights       _rights;
    };

  public:
    void SetPrevious (Stage *previous);

    Stage *GetPreviousStage ();

    const gchar *GetClassName ();

    gchar *GetName ();

    void SetContest (Contest *contest);

    gchar *GetFullName ();

    void SetName (gchar *name);

    Rights GetRights ();

    gboolean Locked ();

    void SetClassificationFilter (Filter *filter);

    virtual void SetInputProvider (Stage *input_provider);

    virtual Stage *GetInputProvider ();

    void Lock (Reason reason);

    void UnLock ();

    GSList *GetResult ();

    void RetrieveAttendees ();

    Player *GetPlayerFromRef (guint ref);

    StageClass *GetClass ();

    void SetStatusCbk (StatusCbk  cbk,
                       void      *data);

    void SetScoreStuffingPolicy (gboolean allowed);

    virtual void Wipe () {};

    virtual void Display () {};

    virtual void Garnish () {};

    virtual void Load (xmlNode *xml_node) {};

    virtual void Save (xmlTextWriter *xml_writer) {};

    virtual void Dump ();

    virtual void ApplyConfig ();

    virtual void FillInConfig ();

    virtual gboolean IsOver ();

    void ToggleClassification (gboolean classification_on);

    static void RegisterStageClass (const gchar *name,
                                    const gchar *xml_name,
                                    Creator      creator,
                                    guint        rights = 0);

    static guint  GetNbStageClass ();

    static StageClass *GetClass (guint index);

    static Stage *CreateInstance (xmlNode *xml_node);

    static Stage *CreateInstance (const gchar *name);

  protected:
    GSList  *_attendees;
    Stage   *_input_provider;
    Contest *_contest;

    Stage (StageClass *stage_class);

    virtual ~Stage ();

    void SignalStatusUpdate ();

    Classification *GetClassification ();

    virtual GSList *GetCurrentClassification () {return NULL;};

    void LockOnClassification (GtkWidget *w);

    virtual void LoadConfiguration (xmlNode *xml_node);

    virtual void SaveConfiguration (xmlTextWriter *xml_writer);

  private:
    static GSList *_stage_base;

    Stage              *_previous;
    StageClass         *_class;
    gchar              *_name;
    gboolean            _locked;
    StageClass         *_stage_class;
    GSList             *_result;
    Classification     *_classification;
    GSList             *_locked_on_classification;
    SensitivityTrigger *_sensitivity_trigger;
    SensitivityTrigger *_score_stuffing_trigger;
    Filter             *_classification_filter;

    void      *_status_cbk_data;
    StatusCbk  _status_cbk;

    void SetResult ();
    void FreeResult ();
    virtual void OnLocked (Reason reason) {};
    virtual void OnUnLocked () {};
    static StageClass *GetClass (const gchar *name);

    void UpdateClassification (GSList *result);
};

#endif