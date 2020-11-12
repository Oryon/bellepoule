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
      let caller = this;

      this.trigger.onclick = function () {caller.onClicked ();};
      this.trigger.addEventListener ('long-press', function (e) {caller.onLongPress (e);});
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

  onClicked ()
  {
    if (this.freezed == false)
    {
      this.onSensorClicked ();
    }
    this.unFreeze ();
  }

  onLongPress (e)
  {
    e.preventDefault ();
    this.freeze ();
    this.onSensorLongPress ();
  }
}
