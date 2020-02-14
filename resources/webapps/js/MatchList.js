class MatchList extends Module
{
  constructor (container)
  {
    super ('MatchList',
           container);
  }

  displayBatch (batch, name)
  {
    batch.display (name, this.container);
  }

  clear ()
  {
    let table = this.container.firstElementChild;

    for (let row of table.rows)
    {
      while (row.hasChildNodes ())
      {
        row.removeChild (row.firstChild);
      }
    }
  }
}
