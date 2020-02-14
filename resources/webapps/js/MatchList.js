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
}
