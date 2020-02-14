class Module
{
  constructor (name,
               container)
  {
    this.name         = name;
    this.container    = container;
    this.container.id = name;
  }

  show ()
  {
    console.log ('show ("' + this.name + "')");
    this.container.style.display = 'block';
  }

  hide ()
  {
    console.log ('hide ("' + this.name + "')");
    this.container.style.display = 'none';
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
