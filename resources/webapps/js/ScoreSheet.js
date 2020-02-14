class ScoreSheet extends Module
{
  constructor (container,
               msg,
               red_fencer,
               green_fencer)
  {
    super ('ScoreSheet',
           container);

    this.clear ();

    this.competition = Message.getField (msg.data, 'Body', 'competition');
    this.stage       = Message.getField (msg.data, 'Body', 'stage');
    this.batch       = Message.getField (msg.data, 'Body', 'batch');
    this.bout        = Message.getField (msg.data, 'Body', 'bout');

    {
      let xml         = Message.getField (msg.data, 'Body', 'xml');
      let parser      = new DOMParser ();
      let xml_doc     = parser.parseFromString (xml, 'text/xml');
      let root        = xml_doc.getElementsByTagName ('CompetitionIndividuelle')[0];
      let competition = new Competition (root);
      let schemes     = ['TableauSuisse/SuiteDeTableaux/Tableau/Match', 'RondeSuisse/Match'];

      competition.addPanel ();

      for (let scheme of schemes)
      {
        let path  = '/CompetitionIndividuelle/' + scheme;
        let nodes = xml_doc.evaluate (path, root, null, XPathResult.ANY_TYPE, null);
        let node  = nodes.iterateNext ();

        if (node && node.childElementCount == 2)
        {
          this.fencers = [red_fencer, green_fencer];

          for (let f = 0; f < 2; f++)
          {
            this.fencers[f].display ();

            this.fencers[f].feed (competition, node.childNodes[f]);
            this.fencers[f].score.setListener (this);
          }
        }
      }
    }

    red_fencer.setOpponent   (green_fencer);
    green_fencer.setOpponent (red_fencer);
  }

  update (xml)
  {
    let parser  = new DOMParser ();
    let xml_doc = parser.parseFromString (xml, 'text/xml');
    let root    = xml_doc.getElementsByTagName ('Match')[0];

    if (root.childNodes.length == 2)
    {
      for (let f = 0; f < 2; f++)
      {
        this.fencers[f].update (root.childNodes[f]);
      }
    }
  }

  onNewScore ()
  {
    let dropped_match = false;
    let xml = '<?xml version="1.0" encoding="UTF-8"?>';
    let msg = new Message ('SmartPoule::Score');

    {
      xml += '<Match>';

      for (let f = 0; f < 2; f++)
      {
        if (   (this.fencers[f].score.status == 'A')
            || (this.fencers[f].score.status == 'E'))
        {
          dropped_match = true;
          break;
        }
      }

      for (let f = 0; f < 2; f++)
      {
        xml += '<Tireur REF="' + this.fencers[f].ref + '"';
        if (dropped_match == false)
        {
          xml += ' Score="' + this.fencers[f].score.points + '"';
        }
        if (this.fencers[f].score.status != null)
        {
          xml += ' Statut="' + this.fencers[f].score.status + '"';
        }
        xml += '/>';
      }
      xml += '</Match>';
    }

    msg.addField ('competition', this.competition);
    msg.addField ('stage',       this.stage);
    msg.addField ('batch',       this.batch);
    msg.addField ('bout',        this.bout);
    msg.addField ('xml',         xml);

    socket.send (msg.data);

    if (mirror != '')
    {
      msg.addField ('mirror', mirror);

      socket.send (msg.data);
    }
  }
}
