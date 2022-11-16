class Batch
{
  constructor (stage,
               id,
               step,
               xml,
               scheme,
               piste)
  {
    let parser = new DOMParser ();

    this.xml_doc = parser.parseFromString (xml, 'text/xml');
    this.root    = this.xml_doc.getElementsByTagName ('CompetitionIndividuelle')[0];
    this.scheme  = scheme;

    this.competition = new Competition (this.root);
    this.stage       = stage;
    this.id          = id;
    this.step        = step;
    this.over        = true;
    this.piste       = piste;

    {
      let nodes = this.getMessageNodes ();
      let node  = nodes.iterateNext ();

      if (node)
      {
        for (let i = 0; i < node.childElementCount; i++)
        {
          let match_over = false;

          for (let f = 0; f < node.childNodes[i].childElementCount; f++)
          {
            let fencer_node = node.childNodes[i].childNodes[f];

            if (   (fencer_node.getAttribute ('Statut') == 'V')
                || (fencer_node.getAttribute ('Statut') == 'A')
                || (fencer_node.getAttribute ('Statut') == 'E'))
            {
              match_over = true;
            }
          }

          if (match_over == false)
          {
            this.over = false;
            return;
          }
        }
      }
    }
  }

  getMessageNodes ()
  {
    return this.xml_doc.evaluate (this.scheme,
                                  this.root,
                                  null,
                                  XPathResult.ANY_TYPE,
                                  null);
  }

  display (what, where, read_only)
  {
    let table = where.firstElementChild;
    let nodes = this.getMessageNodes ();
    let node  = nodes.iterateNext ();

    if (node)
    {
      let context      = this;
      let nb_displayed = 0;

      {
        let header = table.insertRow (-1);
        let cell   = header.insertCell (-1);
        let html   = '</p>';
        let title;

        if (isNaN (what) == false)
        {
          html += '<td><b>' + 'Tour N° ' + what + '</b></td>';
        }
        else if (what == '1:T2')
        {
          html += '<td><b>' + 'Finale' + '</b></td>';
        }
        else if (what == '3:T2')
        {
          html += '<td><b>' + '3ième place' + '</b></td>';
        }
        else if (what == '1:T4')
        {
          html += '<td><b>' + 'Demi-finale' + '</b></td>';
        }
        else if (what == '1:T8')
        {
          html += '<td><b>' + 'Tableau de 8' + '</b></td>';
        }
        else
        {
          html += '<td><b>' + what + '</b></td>';
        }

        cell.colSpan = 6;
        cell.setAttribute ('class', 'td_step');
        cell.innerHTML = html;
      }

      for (let i = 0; i < node.childElementCount; i++)
      {
        let match = node.childNodes[i];

        if (   (this.piste == null)
            || (match.getAttribute ('Piste') == null)
            || (match.getAttribute ('Piste') == this.piste))
        {
          let match_id   = match.getAttribute ('ID');
          let row        = table.insertRow (-1);
          let match_over = false;

          if (read_only == false)
          {
            row.addEventListener ('click', function () {Batch.onClicked (context, match_id);});
          }

          {
            let cell = row.insertCell (-1);
            let html = '<td><b>' + match.getAttribute ('ID') + '</b></td>';

            cell.setAttribute ('class', 'td_match_id');
            cell.innerHTML = html;
          }

          for (let f = 0; f < node.childNodes[i].childElementCount; f++)
          {
            let fencer_node = node.childNodes[i].childNodes[f];
            let fencer_id   = fencer_node.getAttribute ('REF');
            let fencer      = this.competition.getFencer (fencer_id);
            let cell        = row.insertCell (-1);
            let html        = '<td><b>' + fencer.getAttribute ('Nom') + '</b> ' +
                                          fencer.getAttribute ('Prenom') + '</td>';

            if (   (fencer_node.getAttribute ('Statut') == 'V')
                || (fencer_node.getAttribute ('Statut') == 'A')
                || (fencer_node.getAttribute ('Statut') == 'E'))
            {
              match_over = match.getAttribute ('Piste') == null;
            }

            cell.setAttribute ('class', 'td_matchlist');
            cell.innerHTML = html;
          }

          // Empty column
          {
            let cell = row.insertCell (-1);
            let html = '<td></td>';

            cell.innerHTML = html;
          }

          // Club
          for (let f = 0; f < node.childNodes[i].childElementCount; f++)
          {
            let fencer_node = node.childNodes[i].childNodes[f];
            let fencer_id   = fencer_node.getAttribute ('REF');
            let fencer      = this.competition.getFencer (fencer_id);
            let cell        = row.insertCell (-1);
            let html        = '<td>' + fencer.getAttribute ('Club') + '</td>';

            cell.setAttribute ('class', 'td_matchlist');
            cell.innerHTML = html;
          }

          // Empty column
          {
            let cell = row.insertCell (-1);
            let html = '<td></td>';

            cell.innerHTML = html;
          }

          // Score
          for (let f = 0; f < node.childNodes[i].childElementCount; f++)
          {
            let fencer_node = node.childNodes[i].childNodes[f];
            let score       = fencer_node.getAttribute ('Score');
            let cell        = row.insertCell (-1);
            let html        = '';

            if (score)
            {
              html += '<td"><span style="background-color: #000000">&nbsp';
              if (score < 10)
              {
                html += '&nbsp&nbsp';
              }
              html += score;
              html += '&nbsp</span></td>';
            }
            else
            {
              html = '<td"></td>';
            }

            cell.setAttribute ('class', 'td_score');
            cell.innerHTML = html;
          }

          if (match.getAttribute ('Piste') != null)
          {
            let cell = row.insertCell (-1);
            let html = '<td><b>Piste ' + match.getAttribute ('Piste') + '</b></td>';

            cell.setAttribute ('class', 'td_piste');
            cell.innerHTML = html;
          }

          if (match_over)
          {
            row.style.display = 'none';
          }
          else
          {
            row.setAttribute ('class', 'tr_matchlist');
            if (nb_displayed%2 == 0)
            {
              row.style.backgroundColor = 'lightgreen';
            }

            nb_displayed++;
          }
        }
      }
    }
  }

  static onClicked (batch, match_id)
  {
    let msg = new Message ('SmartPoule::ScoreSheetCall');

    msg.addField ('competition', batch.competition.get ('ID'));
    msg.addField ('stage',       batch.stage);

    if (batch.id)
    {
      let match_index = parseInt (match_id) - 1;

      msg.addField ('batch', batch.id);
      msg.addField ('bout',  batch.step + ':' + match_index);
    }
    else
    {
      msg.addField ('bout', match_id);
    }

    socket.send (msg.data);
  }
}
