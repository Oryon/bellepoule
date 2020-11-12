class ModuleStack
{
  constructor (mode,
               cross_container)
  {
    this.stack = new Array ();

    if (mode != 'mirror')
    {
      this.cross = cross_container;
      let svg    = document.getElementById ('close.svg');
      let caller = this;
      let html   = '';

      html += '<img src="data:image/svg+xml;base64,' + svg.dataset.url + '"/>';
      this.cross.innerHTML = html;
      this.cross.onclick = function () {caller.onCrossClicked ();};
    }
  }

  onDirty ()
  {
    let moduleDisplayed = null;

    if (this.cross)
    {
      this.cross.style.display = 'none';
    }

    for (let i = this.stack.length; i > 0; i--)
    {
      let module = this.stack[i-1];

      if (moduleDisplayed == null)
      {
        if (module.isEmpty () == false)
        {
          module.show ();
          moduleDisplayed = module;

          if (this.cross && (this.stack.length > 2))
          {
            this.cross.style.display = 'block';
          }
        }
        else
        {
          module.hide ();
        }
      }
      else
      {
        module.hide ();
      }
    }
  }

  push (module)
  {
    module.setListener (this);
    this.stack.push (module);
    this.onDirty ();
  }

  pull (module)
  {
    if (module)
    {
      let index = this.stack.indexOf (module);

      if (index != -1)
      {
        this.stack.splice (index, 1);
      }

      module.setListener (null);
      module.hide ();

      this.onDirty ();
    }
  }

  restore (module)
  {
    while (this.stack.length > 0)
    {
      let current = this.stack[this.stack.length-1];

      if (current == module)
      {
        break;
      }
      this.pull (current);
    }
  }

  onCrossClicked ()
  {
    let length = this.stack.length;

    if (length > 0)
    {
      this.pull (this.stack[length-1]);
    }
  }
}
