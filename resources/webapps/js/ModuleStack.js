class ModuleStack
{
  constructor (mode)
  {
    this.stack = new Array ();

    if (mode != 'mirror')
    {
      this.cross = document.getElementById ('cross');
      let svg     = document.getElementById ('close.svg');
      let context = this;
      let html    = '';

      html += '<img src="data:image/svg+xml;base64,' + svg.dataset.url + '"/>';
      this.cross.innerHTML = html;
      this.cross.onclick = function () {ModuleStack.onCrossClicked (context);};
    }
  }

  push (module)
  {
    if (this.stack.length > 0)
    {
      this.stack[this.stack.length-1].hide ();
    }

    this.stack.push (module);
    module.show ();

    if (this.cross && (this.stack.length > 1))
    {
      this.cross.style.display = 'block';
    }
  }

  pull (module)
  {
    if (module)
    {
      let index = this.stack.indexOf (module);

      if (index != -1)
      {
        if ((index > 0) && (index == this.stack.length-1))
        {
          this.stack[index-1].show ();
        }

        this.stack.splice (index, 1);

        if (this.cross && (this.stack.length == 1))
        {
          this.cross.style.display = 'none';
        }
      }

      module.hide ();
    }
  }

  static onCrossClicked (what)
  {
    let length = what.stack.length;

    if (length > 0)
    {
      what.pull (what.stack[length-1]);
    }
  }
}
