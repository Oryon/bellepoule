class Module
{
  constructor (name,
               container)
  {
    this.name      = name;
    this.container = container;
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
}
