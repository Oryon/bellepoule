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
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <gtk/gtk.h>

#include "object.hpp"
#include "data.hpp"
#include "sensitivity_trigger.hpp"
#include "attendees.hpp"

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
      STAGE_VIEW_UNDEFINED,
      STAGE_VIEW_RESULT,
      STAGE_VIEW_CLASSIFICATION,

      N_STAGE_VIEW
    } StageView;

    typedef enum
    {
      EDITABLE = 0x0001
    } Rights;

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

    Stage *GetNextStage ();

    const gchar *GetClassName ();

    gchar *GetName ();

    void SetContest (Contest *contest);

    Contest *GetContest ();

    gchar *GetFullName ();

    void SetName (gchar *name);

    void SetId (guint id);

    guint GetId ();

    guint32 GetRandSeed ();

    void SetRandSeed (guint32 rand_seed);

    Rights GetRights ();

    gboolean Locked ();

    Data *GetMaxScore ();

    void SetClassificationFilter (Filter *filter);

    virtual const gchar *GetInputProviderClient ();

    virtual void SetInputProvider (Stage *input_provider);

    virtual Stage *GetInputProvider ();

    void Lock ();

    void UnLock ();

    GSList *GetResult ();

    void RetrieveAttendees ();

    virtual GSList *GetOutputShortlist ();

    Player *GetFencerFromRef (guint ref);

    StageClass *GetClass ();

    void SetStatusCbk (StatusCbk  cbk,
                       void      *data);

    void SetScoreStuffingPolicy (gboolean allowed);

    virtual void Wipe () {};

    virtual void Display () {};

    virtual void Garnish () {};

    virtual void Load (xmlNode *xml_node);

    virtual void Load (xmlXPathContext *xml_context,
                       const gchar     *from_node) {};

    virtual void OnLoadingCompleted () {};

    virtual void Save (xmlTextWriter *xml_writer) {};

    virtual void Dump ();

    virtual void ApplyConfig ();

    virtual void FillInConfig ();

    virtual gboolean IsOver ();

    void ToggleClassification (gboolean classification_on);

    void ActivateNbQualified ();

    void DeactivateNbQualified ();

  public:
    static void RegisterStageClass (const gchar *name,
                                    const gchar *xml_name,
                                    Creator      creator,
                                    guint        rights = 0);

    static guint  GetNbStageClass ();

    static StageClass *GetClass (guint index);

    static Stage *CreateInstance (xmlNode *xml_node);

    static Stage *CreateInstance (const gchar *name);

  protected:
    Attendees *_attendees;
    Stage     *_input_provider;
    Contest   *_contest;
    Data      *_max_score;
    Data      *_nb_qualified;

    Stage (StageClass *stage_class);

    virtual ~Stage ();

    void SignalStatusUpdate ();

    Classification *GetClassification ();

    virtual GSList *GetCurrentClassification () {return NULL;};

    void LockOnClassification (GtkWidget *w);

    virtual void LoadConfiguration (xmlNode *xml_node);

    virtual void SaveConfiguration (xmlTextWriter *xml_writer);

    virtual void SaveAttendees (xmlTextWriter *xml_writer);

    void LoadAttendees (xmlNode *n);

    StageView GetStageView (GtkPrintOperation *operation);

  private:
    static GSList *_stage_base;

    Stage              *_previous;
    Stage              *_next;
    StageClass         *_class;
    gchar              *_name;
    guint               _id;
    gboolean            _locked;
    StageClass         *_stage_class;
    GSList             *_result;
    Classification     *_classification;
    GSList             *_locked_on_classification;
    SensitivityTrigger  _sensitivity_trigger;
    SensitivityTrigger *_score_stuffing_trigger;
    gboolean            _classification_on;

    void      *_status_cbk_data;
    StatusCbk  _status_cbk;

    void SetResult ();
    void FreeResult ();
    virtual void OnLocked () {};
    virtual void OnUnLocked () {};
    static StageClass *GetClass (const gchar *name);

    void UpdateClassification (GSList *result);

    Object *GetPlayerDataOwner ();

    void UpdateNbQualifiedAdjustment ();
};

#endif
