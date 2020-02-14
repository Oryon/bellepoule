class MatchList extends Module
{
  constructor (hook)
  {
    super ('MatchList',
           document.createElement ('div'));

    hook.appendChild (this.container);
    this.hide ();

    {
      let table = document.createElement ('table');

      table.className = 'table_matchlist';
      this.container.appendChild (table);
    }
  }

  displayBatch (batch, name)
  {
    batch.display (name, this.container);
  }
}
