class MatchList extends Module
{
  constructor (hook, read_only)
  {
    super ('MatchList',
           hook);

    this.empty     = true;
    this.read_only = read_only;

    {
      let table = document.createElement ('table');

      table.className = 'table_matchlist';
      this.container.appendChild (table);
    }
  }

  clear ()
  {
    super.clear ();
    this.empty = true;
    this.onDirty ();
  }

  isEmpty ()
  {
    return this.empty;
  }

  displayBatch (batch, name)
  {
    batch.display (name, this.container, this.read_only);
    this.empty = false;
    this.onDirty ();
  }
}
