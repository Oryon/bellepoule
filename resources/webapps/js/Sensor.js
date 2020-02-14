class Sensor {
  constructor ()
  {
    this.freezed = false;
  }

  setTrigger (trigger)
  {
    this.trigger = trigger;
    if (this.trigger != null)
    {
      let context = this;

      this.trigger.onclick = function () {Sensor.onClicked (context);};
      this.trigger.addEventListener ('long-press', function (e) {Sensor.onLongPress (context, e);});
    }
  }

  freeze ()
  {
    if (typeof window.orientation == 'undefined')
    {
      this.freezed = true;
    }
  }

  unFreeze ()
  {
    this.freezed = false;
  }

  static onClicked (what)
  {
    if (what.freezed == false)
    {
      what.onSensorClicked (what);
    }
    what.unFreeze ();
  }

  static onLongPress (what, e)
  {
    e.preventDefault ();
    what.freeze ();
    what.onSensorLongPress (what);
  }
}
