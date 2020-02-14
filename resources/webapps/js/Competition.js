class Competition
{
  constructor (xml)
  {
    this.xml     = xml;
    this.fencers = this.xml.getElementsByTagName ('Tireurs')[0];
  }

  get (attribute)
  {
    return this.xml.getAttribute (attribute);
  }

  getFencer (id)
  {
    for (let f = 0; f < this.fencers.childElementCount; f++)
    {
      let current = this.fencers.children[f];

      if (current.getAttribute ('ID') == id)
      {
        return current;
      }
    }
  }

  addPanel ()
  {
    let weapon  = {'S':'Sabre', 'E':'Epée', 'F':'Fleuret', 'X':'Educ', 'L':'Laser'};
    let gender  = {'M':'Homme', 'F':'Dame', 'X':'Mixte'};
    let lastRow = document.getElementById ('competition.panel');
    let cell    = lastRow.insertCell (-1);
    let html    = '';

    html += '</td>';
    html += '<td>';
    html += '  <div class="spacer">';
    html += '&nbsp';
    html += '  </div>';
    html += '  <div class="competition">';
    html +=     weapon[this.get ('Arme')] + ' '
      html +=     gender[this.get ('Sexe')] + ' ';
    html +=     this.get ('Categorie');
    html += '  </div>';
    html += '</td>';
    cell.innerHTML = html;

    cell.colSpan = 12;
    cell.style.background = this.get ('Couleur');
  }
};
