class Card extends Sensor
{
  constructor (listener,
               cell,
               html,
               code)
  {
    super ();

    this.code     = code;
    this.listener = listener;

    cell.innerHTML = html;
    cell.colSpan   = 2;
    this.setTrigger (cell);
  }

  reset ()
  {
    this.listener.onCardCancelled (this.code);
  }

  onSensorClicked (sensor)
  {
    this.listener.onCardClicked (this.code);
  }

  onSensorLongPress (sensor)
  {
    this.reset ();
  }
}
