class MatchList extends Module
{
  constructor (hook)
  {
    super ('MatchList',
           document.createElement ('div'));

    this.empty = true;

    hook.appendChild (this.container);

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
    super.onUpdated ();
  }

  isEmpty ()
  {
    return this.empty;
  }

  displayBatch (batch, name)
  {
    batch.display (name, this.container);
    this.empty = false;
    super.onUpdated ();
  }
}
