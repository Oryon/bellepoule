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

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <gtk/gtk.h>

#include "util/object.hpp"
#include "util/anti_cheat_block.hpp"
#include "network/advertiser.hpp"
#include "error.hpp"

class Player;
class Match;
class Classification;
class Filter;
class Contest;
class Data;
class SensitivityTrigger;
class Attendees;
class XmlScheme;

class Stage : public virtual Object,
              public Net::Advertiser::Feeder,
              public AntiCheatBlock
{
  public:
    struct StageClass;

    typedef Stage * (*Creator) (StageClass *stage_class);

    struct Listener
    {
      virtual void OnStageStatusChanged (Stage *stage) = 0;
    };

    enum class StageView
    {
      UNDEFINED,
      RESULT,
      CLASSIFICATION,

      N_STAGE_VIEW
    };

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

    const gchar *GetPurpose ();

    gchar *GetName ();

    void SetContest (Contest *contest);

    Contest *GetContest ();

    virtual void ShareAttendees (Stage *with);

    gchar *GetFullName ();

    void SetName (gchar *name);

    void SetId (guint id);

    guint GetId ();

    guint GetNetID ();

    guint32 GetAntiCheatToken () override;

    void SetAntiCheatToken (guint32 token);

    void RestoreAntiCheatToken (guint32 token); // !!

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

    void SetStatusListener (Listener *listener);

    void SetScoreStuffingPolicy (gboolean allowed);

    virtual void SetTeamEvent (gboolean team_event) {};

    virtual void Reset ();

    virtual void Display ();

    virtual void Garnish () {};

    virtual void Load (xmlNode *xml_node);

    virtual void Load (xmlXPathContext *xml_context,
                       const gchar     *from_node) {};

    virtual void OnLoadingCompleted () {};

    virtual void Save (XmlScheme *xml_scheme);

    void Dump () override;

    virtual void ApplyConfig ();

    virtual void FillInConfig ();

    virtual gboolean IsOver ();

    virtual Error *GetError ();

    void ToggleClassification (gboolean classification_on);

    void ActivateNbQualified ();

    void DeactivateNbQualified ();

    const gchar *GetXmlPlayerTag ();

    void OnFilterClicked (const gchar *classification_toggle_button);

    virtual GList *GetBookSections (StageView view);

  public:
    static void RegisterStageClass (const gchar *name,
                                    const gchar *xml_name,
                                    Creator      creator,
                                    guint        rights = 0);

    static guint  GetNbStageClass ();

    static StageClass *GetClass (guint index);

    static Stage *CreateInstance (const gchar *name);

    void LoadMatch (xmlNode *xml_node,
                    Match   *match);

  protected:
    Attendees *_attendees;
    Stage     *_input_provider;
    Contest   *_contest;
    Data      *_max_score;
    Data      *_nb_qualified;

    Stage (StageClass *stage_class);

    ~Stage () override;

    void SignalStatusUpdate ();

    GSList *GetShortList ();

    void RefreshClassification ();

    Classification *GetClassification ();

    void LockOnClassification (GtkWidget *w);

    virtual void LoadConfiguration (xmlNode *xml_node);

    virtual void SaveConfiguration (XmlScheme *xml_scheme);

    virtual void SaveAttendees (XmlScheme *xml_scheme);

    void LoadAttendees (xmlNode *n);

    StageView GetStageView (GtkPrintOperation *operation);

    void UpdateClassification (Classification *classification,
                               GSList         *result);

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
    SensitivityTrigger *_sensitivity_trigger;
    SensitivityTrigger *_score_stuffing_trigger;
    gboolean            _classification_on;
    guint32             _anti_cheat_token;

    Listener *_status_listener;

    void SetResult ();
    void FreeResult ();
    virtual void OnLocked () {};
    virtual void OnUnLocked () {};
    static StageClass *GetClass (const gchar *name);

    virtual GSList *GetCurrentClassification () {return nullptr;};

    virtual void SetOutputShortlist ();

    virtual gboolean HasItsOwnRanking ();

    Object *GetPlayerDataOwner ();

    void InitQualifiedForm ();

    gchar *GetAnnounce () override;

    void SaveFilters (XmlScheme   *xml_scheme,
                      const gchar *as = "");
};
