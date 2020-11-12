class Module
{
  constructor (name,
               container)
  {
    this.name          = name;
    this.container     = container;
    this.listener      = null;
    this.slave         = null;

    this.scroll_handler = null;
    this.scroll_speed   = 0;

    this.hide ();
  }

  setListener (listener)
  {
    this.listener = listener;
  }

  setScrollSpeed (speed)
  {
    this.scroll_speed = speed;
  }

  onDirty ()
  {
    if (this.listener != null)
    {
      this.listener.onDirty ();
    }
  }

  restartScroll ()
  {
    let caller = this;

    window.scrollTo (0, 0);
    this.scroll_handler = setTimeout (function () {caller.scroll ();}, 1000);
  }

  scroll ()
  {
    let caller          = this;
    let before_position = window.scrollY;

    window.scrollBy (0, 1);
    if (before_position == window.scrollY)
    {
      this.scroll_handler = setTimeout (function () {caller.restartScroll ();}, 5000);
    }
    else
    {
      this.scroll_handler = setTimeout (function () {caller.scroll ();}, this.scroll_speed);
    }
  }

  show ()
  {
    this.container.style.display = 'block';

    if (this.slave != null)
    {
      this.slave.show ();
    }

    if (this.scroll_speed > 0)
    {
      let caller = this;

      this.scroll_handler = setTimeout (function () {caller.scroll ();}, this.scroll_speed);
    }
  }

  hide ()
  {
    if (this.scroll_handler != null)
    {
      clearInterval (this.scroll_handler);
      this.scroll_handler = null;
    }

    this.container.style.display = 'none';

    if (this.slave != null)
    {
      this.slave.hide ();
    }
  }

  attach (slave)
  {
    this.slave = slave;
  }

  isEmpty ()
  {
    return false;
  }

  clear ()
  {
    let table = this.container.firstElementChild;

    for (let panel of table.rows)
    {
      while (panel.hasChildNodes ())
      {
        panel.removeChild (panel.firstChild);
      }
    }
  }
}
