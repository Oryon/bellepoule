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

#include <libxml/xmlwriter.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <gtk/gtk.h>

#include "util/object.hpp"
#include "util/data.hpp"
#include "util/sensitivity_trigger.hpp"
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
      EDITABLE  = 0x0001,
      REMOVABLE = 0x0002
    } Rights;

    struct StageClass
    {
      const gchar *_name;
      const gchar *_xml_name;
      Creator      _creator;
      guint        _rights;
    };

  public:
    void SetPrevious (Stage *previous);

    Stage *GetPreviousStage ();

    Stage *GetNextStage ();

    const gchar *GetKlassName ();

    gchar *GetName ();

    void SetContest (Contest *contest);

    Contest *GetContest ();

    gchar *GetFullName ();

    void SetName (gchar *name);

    void SetId (guint id);

    guint GetId ();

    guint32 GetRandSeed ();

    void SetRandSeed (guint32 rand_seed);

    guint GetRights ();

    gboolean Locked ();

    Data *GetMaxScore ();

    void SetClassificationFilter (Filter *filter);

    virtual const gchar *GetInputProviderClient ();

    virtual void SetInputProvider (Stage *input_provider);

    virtual Stage *GetInputProvider ();

    void Lock ();

    void UnLock ();

    void RetrieveAttendees ();

    void GiveEliminatedAFinalRank ();

    guint GetQuotaExceedance ();

    Player *GetFencerFromRef (guint ref);

    StageClass *GetClass ();

    void SetStatusCbk (StatusCbk  cbk,
                       void      *data);

    void SetScoreStuffingPolicy (gboolean allowed);

    virtual void SetTeamEvent (gboolean team_event) {};

    virtual void Reset ();

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

    virtual gchar *GetError ();

    void ToggleClassification (gboolean classification_on);

    void ActivateNbQualified ();

    void DeactivateNbQualified ();

    const gchar *GetXmlPlayerTag ();

    virtual gboolean OnHttpPost (const gchar *command,
                                 const gchar **ressource,
                                 const gchar *data);

    void OnFilterClicked (const gchar *classification_toggle_button);

    virtual void DrawConfig (GtkPrintOperation *operation,
                             GtkPrintContext   *context,
                             gint               page_nr);

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

    void UpdateClassification (Classification *classification,
                               GSList         *result);

    void DrawConfigLine (GtkPrintOperation *operation,
                         GtkPrintContext   *context,
                         const gchar       *line);

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
    GSList             *_output_short_list;
    guint               _quota_exceedance;
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

    virtual void SetOutputShortlist ();

    virtual gboolean HasItsOwnRanking ();

    Object *GetPlayerDataOwner ();

    void InitQualifiedForm ();
};
