class Light extends Counter {
  constructor (trigger, countable, step)
  {
    super (trigger, countable, step);

    this.listeners = [];
  }

  onSensorClicked (sensor)
  {
  }

  cancel ()
  {
    this.countable.reset ();

    this.listeners.forEach (listener => listener.reset ());
  }

  addListener (counter)
  {
    this.listeners.push (counter);
  }
};
