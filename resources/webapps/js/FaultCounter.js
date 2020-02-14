class FaultCounter extends Counter
{
  constructor (trigger,
               penalties)
  {
    super (trigger, new Score (trigger), 1);

    this.penalties = penalties;
  }

  setCountable (countable)
  {
    this.counter1 = new Counter (null, countable, this.penalties[0]);
    this.counter2 = new Counter (null, countable, this.penalties[1]);
  }

  reset ()
  {
    this.counter2.reset ();
    this.counter1.reset ();

    this.reStart ();
  }

  count ()
  {
    let counter;

    if (this.counter1.hits > 0)
    {
      counter = this.counter2;
    }
    else
    {
      counter = this.counter1;
    }

    if (counter.count ())
    {
      super.count ();
    }
  }

  cancel ()
  {
    if (this.counter2.hits > 0)
    {
      this.counter2.cancel ();
    }
    else
    {
      this.counter1.cancel ();
    }

    if (counter.cancel ())
    {
      super.cancel ();
    }
  }
};
