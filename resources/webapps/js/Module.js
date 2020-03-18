class Module
{
  constructor (name,
               container)
  {
    this.name      = name;
    this.container = container;
    this.listener  = null;
    this.slave     = null;
  }

  setListener (listener)
  {
    this.listener = listener;
  }

  onUpdated ()
  {
    if (this.listener != null)
    {
      this.listener.onDirty ();
    }
  }

  show ()
  {
    this.container.style.display = 'block';

    if (this.slave != null)
    {
      this.slave.show ();
    }
  }

  hide ()
  {
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
