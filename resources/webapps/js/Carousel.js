class Carousel extends Module
{
  constructor ()
  {
    super ('Carousel',
           document.createElement ('div'));

    this.modules  = [];
    this.current  = -1;
    this.interval = null;
  }

  manage (module)
  {
    this.modules.push (module);
    module.setListener (this);

    if (this.current == -1)
    {
      this.current = 0;
    }
  }

  show ()
  {
    if (this.current != -1)
    {
      for (let module of this.modules)
      {
        module.hide ();
      }

      this.modules[this.current].show ();
    }

    if (this.interval == null)
    {
      let caller = this;

      this.interval = setInterval (function () {caller.onTic ();}, 30000);
    }
  }

  hide ()
  {
    if (typeof (this.current) != 'undefined')
    {
      if (this.current != -1)
      {
        this.modules[this.current].hide ();
      }
    }

    if (this.interval == null)
    {
      clearInterval (this.interval);
      this.interval = null;
    }
  }

  isEmpty ()
  {
    if (this.current != -1)
    {
      return this.modules[this.current].isEmpty ();
    }
  }

  onTic ()
  {
    this.current++;
    if (this.current == this.modules.length)
    {
      this.current = 0;
    }

    this.onDirty ()
  }
}
