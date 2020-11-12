class Classification extends Module
{
  constructor (hook)
  {
    super ('Classification',
           hook);

    this.competition = null;
    this.xml         = null;

    {
      let table = document.createElement ('table');

      table.className = 'table_classification';
      this.container.appendChild (table);
    }

    this.setScrollSpeed (25);
  }

  setCompetition (competition)
  {
    this.competition = competition;

    if (this.xml)
    {
      this.feed (this.xml);
    }
  }

  feed (xml)
  {
    this.xml = xml;

    if (this.competition)
    {
      this.clear ();

      let table = this.container.firstElementChild;

      let parser  = new DOMParser ();
      let xml_doc = parser.parseFromString (this.xml, 'text/xml');
      let root    = xml_doc.getElementsByTagName ('Classement')[0];

      {
        let header = table.insertRow (-1);
        let cell   = header.insertCell (-1);
        let html   = '</p>';

          html += '<td><b>' + 'Classement' + '</b></td>';

        cell.colSpan = 4;
        cell.setAttribute ('class', 'td_step');
        cell.innerHTML = html;
      }

      for (let i = 0; i < root.childNodes.length; i++)
      {
        let row    = table.insertRow (-1);
        let node   = root.childNodes[i];
        let fencer = this.competition.getFencer (node.getAttribute ('Ref'));

        {
          let cell = row.insertCell (-1);
          let html = '<td><b>' + (parseInt (i, 10)+1) + '</b></td>';

          cell.setAttribute ('class', 'td_classification');
          cell.innerHTML = html;
        }

        {
          let cell = row.insertCell (-1);
          let html = '<td>' + fencer.getAttribute ('Nom') + '</td>';

          cell.setAttribute ('class', 'td_classification');
          cell.innerHTML = html;
        }

        {
          let cell = row.insertCell (-1);
          let html = '<td>' + fencer.getAttribute ('Prenom') + '</td>';

          cell.setAttribute ('class', 'td_classification');
          cell.innerHTML = html;
        }

        {
          let cell = row.insertCell (-1);
          let html = '<td>' + 'Elo ' + node.getAttribute ('Elo') + '</td>';

          cell.setAttribute ('class', 'td_classification');
          cell.innerHTML = html;
        }

        if (i%2 == 0)
        {
          row.style.backgroundColor = 'lightgreen';
        }
      }
    }
  }
}
