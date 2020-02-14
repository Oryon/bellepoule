class Counter extends Sensor {
  constructor (trigger,
               countable,
               step)
  {
    super ();

    this.countable = countable;
    this.step      = step;
    this.hits      = 0;

    this.setTrigger (trigger);
  }

  reset ()
  {
    this.hits = 0;
  }

  reStart ()
  {
    this.hits = 0;
    this.countable.reset ();
  }

  count ()
  {
    if (this.step >= 0)
    {
      this.hits++;
      return this.countable.increment (this.step);
    }
    return false;
  }

  onSensorClicked (sensor)
  {
    this.count ();
  }

  onSensorLongPress (sensor)
  {
    this.cancel ();
  }

  cancel ()
  {
    if (this.hits > 0)
    {
      this.hits--;
      return this.countable.increment (-this.step);
    }
    return false;
  }
};
