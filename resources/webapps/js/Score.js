class Score {
  constructor (display)
  {
    this.status         = null;
    this.opponent_score = null;

    this.display = display;
    this.reset ();
  }

  setOpponentScore (opponent_score)
  {
    this.opponent_score = opponent_score;
  }

  setListener (listener)
  {
    this.listener = listener;
  }

  increment (delta)
  {
    if (this.status != null)
    {
      return false;
    }
    if (this.opponent_score != null)
    {
      if (this.opponent_score.status != null)
      {
        return false;
      }
    }

    this.points += delta;
    if (this.points >= 15)
    {
      this.status = 'V';
    }

    this.refresh ();

    return true;
  }

  setPoints (points)
  {
    if (points != null)
    {
      this.points = Number (points);
      this.refresh ();
    }
  }

  setStatus (status)
  {
    this.status = status;
    this.refresh ();
  }

  reset ()
  {
    this.points = 0
      this.refresh ();
  }

  refresh ()
  {
    let row_size = this.display.children[0].getBoundingClientRect().height;

    if (this.status == 'A')
    {
      let svg = document.getElementById ('withdrawal.svg');

      this.display.children[0].innerHTML = '<img src="data:image/svg+xml;base64,' + svg.dataset.url + '" width="' + row_size + '"/>';
    }
    else if (this.status == 'E')
    {
      let svg = document.getElementById ('blackcard.svg');

      this.display.children[0].innerHTML = '<img src="data:image/svg+xml;base64,' + svg.dataset.url + '" width="' + row_size*.7 + '"/>';
    }
    else if (this.status == 'V')
    {
      this.display.children[0].innerHTML = this.status + this.points
    }
    else
    {
      this.display.children[0].innerHTML = this.points
    }

    if (this.listener)
    {
      this.listener.onNewScore ();
    }
  }
}
