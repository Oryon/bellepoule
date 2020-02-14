class Fencer
{
  constructor (color)
  {
    this.rgb           = {'red':'#8F1818', 'green':'#098709'};
    this.color         = color;
    this.freezed       = false;
    this.taintedPanels = [];
    this.faults        = [{'color':'white',  'penalties':[0, 3]},
    {'color':'orange', 'penalties':[3, 5]},
    {'color':'red',    'penalties':[5, -1]}];

    this.opponent = null;
  }

  getDisplaySize ()
  {
    if (mode == 'mirror')
    {
      return 'big';
    }
    else
    {
      return 'small';
    }
  }

  setOpponent (opponent)
  {
    this.opponent = opponent;

    if (opponent != null)
    {
      if ((this.score.status == 'D') && (opponent.score.status == 'D'))
      {
        this.score.status     = null;
        opponent.score.status = null;
      }
    }

    for (let i = 0; i < this.faults.length; i++)
    {
      if (this.faults[i].counter != null)
      {
        this.faults[i].counter.setCountable (opponent.score);
        this.light.addListener (opponent.faults[i].counter);
      }
    }

    this.light.addListener (this.withdrawal_card);
    this.light.addListener (this.excluded_card);

    this.score.setOpponentScore (opponent.score);
  }

  onCardClicked (code)
  {
    if (this.score.status == code)
    {
      this.onCardCancelled ();
    }
    else if (code == 'V')
    {
      if (this.score.points >= this.opponent.score.points)
      {
        this.score.setStatus          ('V');
        this.opponent.score.setStatus ('D');
      }
    }
    else
    {
      this.score.setStatus (code);
    }
  }

  onCardCancelled (code)
  {
    this.score.setStatus          (null);
    this.opponent.score.setStatus (null);
  }

  display ()
  {
    this.addIdentityPanel ('name');
    this.addIdentityPanel ('firstname');
    this.addLightPanel    (this.color);

    if (mode != 'mirror')
    {
      this.addPointsPanel ();
      this.addCardsPanel  ();
      this.addFaultsPanel ();
    }
  }

  update (node)
  {
    this.score.setPoints (node.getAttribute ('Score'));
    this.score.setStatus (node.getAttribute ('Statut'));
  }

  feed (competition, node)
  {
    let name      = document.getElementById (this.color + '-fencer-name');
    let firstname = document.getElementById (this.color + '-fencer-firstname');

    this.taintedPanels.forEach (panel => panel.style.background = competition.get ('Couleur'));

    this.ref = node.getAttribute ('REF');
    name.innerHTML      = competition.getFencer (this.ref).getAttribute('Nom');
    firstname.innerHTML = competition.getFencer (this.ref).getAttribute('Prenom');

    this.score.setPoints (node.getAttribute ('Score'));
    this.score.setStatus (node.getAttribute ('Statut'));

    {
      let svg      = document.getElementById ('close.svg');
      let div      = document.getElementById ('overlay');
      let row_size = name.getBoundingClientRect().height;
      let html     = '';

      html += '<img onclick="CloseScorekeeper ();" src="data:image/svg+xml;base64,' + svg.dataset.url + '" width="' + row_size*1.0 + '"/>';
      div.innerHTML = html;
    }
  }

  addIdentityPanel (what)
  {
    let lastRow = document.getElementById (what + 's.panel');
    let cell    = lastRow.insertCell (-1);
    let html    = '';

    html += '<td>';
    html += '  <div id="' + this.color + '-fencer-' + what + '" class="' + this.getDisplaySize () + '-fencer-' + what + '">';
    html +=     what;
    html += '  </div>';
    html += '</td>';
    cell.innerHTML = html;

    cell.colSpan = 6;

    this.taintedPanels.push (cell);
  }

  addLightPanel (color)
  {
    let lastRow = document.getElementById ('lights.panel');
    let cell    = lastRow.insertCell (-1);
    let html    = '';

    html += '<td>';
    html += '  <div class="' + this.getDisplaySize () + '-score">0</div>';
    html += '</td>';
    cell.innerHTML = html;

    cell.colSpan = 6;
    cell.style.backgroundColor = this.rgb[color]

      this.score = new Score (cell);
    this.light = new Light (cell, this.score, 1);
  }

  addPointsPanel ()
  {
    let lastRow = document.getElementById ('points.panel');
    let points  = [1, 3, 5];

    for (let point of points)
    {
      let cell = lastRow.insertCell (-1);
      let html = '';

      html += '<td>';
      html += '  <div class="point">';
      html +=     point;
      html += '  </div>';
      html += '</td>';
      cell.innerHTML = html;

      cell.colSpan = 2;

      this.light.addListener (new Counter (cell,
                                           this.score,
                                           point));
    }
  }

  addCardsPanel ()
  {
    let svg             = document.getElementById ('withdrawal.svg');
    let lastRow         = document.getElementById ('cards.panel');
    let withdrawal_cell = lastRow.insertCell (-1);
    let victory_cell    = lastRow.insertCell (-1);
    let excluded_cell   = lastRow.insertCell (-1);
    let context         = this;
    let row_size;

    this.victory_card = new Card (this,
                                  victory_cell,
                                  '<td><div class="point">V</div></td>',
                                  'V');
    row_size = victory_cell.getBoundingClientRect().height;

    this.withdrawal_card = new Card (this,
                                     withdrawal_cell,
                                     '<td><img src="data:image/svg+xml;base64,' + svg.dataset.url + '" width="' + row_size*1.0 + '"/></td>',
                                     'A');

    svg = document.getElementById ('blackcard.svg');
    this.excluded_card = new Card (this,
                                   excluded_cell,
                                   '<td><img src="data:image/svg+xml;base64,' + svg.dataset.url + '" width="' + row_size*.7 + '"/></td>',
                                   'E');
  }

  addFaultsPanel ()
  {
    for (let fault of this.faults)
    {
      let lastRow = document.getElementById ('faults-' + fault.color + '.panel');
      let html    = '';
      let cell;

      lastRow.insertCell (-1);
      lastRow.insertCell (-1);
      cell = lastRow.insertCell (-1);

      html += '<td>';
      html += '  <div class="fault" style="background:' + fault.color + ';"></div>';
      html += '</td>';
      cell.innerHTML = html;

      lastRow.insertCell (-1);
      lastRow.insertCell (-1);
      lastRow.insertCell (-1);

      fault.counter = new FaultCounter (cell,
                                        fault.penalties);
    }
  }
}
