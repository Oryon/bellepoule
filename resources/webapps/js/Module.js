class Module
{
  constructor (name,
               container)
  {
    this.name         = name;
    this.container    = container;
    this.container.id = name;
    this.listener     = null;
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
  }

  hide ()
  {
    this.container.style.display = 'none';
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
