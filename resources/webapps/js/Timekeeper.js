class TimekeeperSensor extends Sensor
{
  constructor (listener,
               trigger)
  {
    super ();

    this.setTrigger (trigger);
    this.listener = listener;
  }

  onSensorClicked (sensor)
  {
    this.listener.onClicked ();
  }

  onSensorLongPress (sensor)
  {
    this.listener.stop ();
  }
}

class Timekeeper extends Module
{
  constructor (container)
  {
    super ('Timekeeper',
           container);

    this.duration   = 3*60;
    this.interval   = null;
    this.start_time = null;

    {
      let html = '';

      html += '<div id="timekeeper-control" class="timekeeper-control">'
      html += '</div>';
      html += '<div id="timekeeper-display" class="timekeeper-display">'
      html += '  <div id="timekeeper-MMSS" class="big-7segments"></div>';
      html += '  <div id="timekeeper-ms" class="small-7segments"></div>';
      html += '</div>';

      this.container.innerHTML = html;
    }

    this.MMSS          = document.getElementById ('timekeeper-MMSS');
    this.ms            = document.getElementById ('timekeeper-ms');
    this.display       = document.getElementById ('timekeeper-display');
    this.control_panel = document.getElementById ('timekeeper-control');

    {
      let durations = [{'seconds':3*60, 'color':'#607D3B'},
                       {'seconds':30,   'color':'#c6be00'}];

      this.addDurations (this.control_panel,
                         durations);
    }

    this.sensor = new TimekeeperSensor (this,
                                        document.getElementById ('timekeeper-display'));

    this.refresh (this.duration*1000);
  }

  addDurations (control_panel,
                durations)
  {
    let context = this;
    let html    = '';

    for (let i = 0; i < durations.length; i++)
    {
      let seconds = durations[i].seconds;
      let color   = durations[i].color;

      html += '<div id="timekeeper-' + seconds + '" class="timekeeper-button" style="background-color:' + color + ';">';
      if (seconds < 60)
      {
        html += seconds + ' sec';
      }
      else
      {
        html += seconds/60 + ' min';
      }
      html += '</div>';
    }

    control_panel.innerHTML += html;

    for (let i = 0; i < durations.length; i++)
    {
      let seconds = durations[i].seconds;
      let element = document.getElementById ('timekeeper-' + seconds);

      element.onclick = function () {Timekeeper.onDurationSelected (context,
                                                                    element);};
    }
  }

  refresh (duration)
  {
    let date     = new Date (duration);
    let minutes  = date.getMinutes ();
    let seconds  = date.getSeconds ();
    let millisec = date.getMilliseconds ();
    let ss       = '';
    let mm       = '';
    let ms       = '';

    if (minutes <= 9)
    {
      mm += '0';
    }
    mm += minutes;

    if (seconds <= 9)
    {
      ss += '0';
    }
    ss += seconds;

    ms += Math.floor (millisec/100);

    this.MMSS.innerHTML = mm + ':' + ss;
    this.ms.innerHTML   = '.' + ms;
  }

  start ()
  {
    let context = this;

    if (this.start_time == null)
    {
      let now = new Date ().getTime ();

      this.start_time = now;
    }
    else
    {
      let now = new Date ().getTime ();

      this.start_time += now - this.pause_time;
    }

    this.interval = setInterval (function () {Timekeeper.onTic (context);}, 100);
    this.control_panel.style.display = 'none';
  }

  pause ()
  {
    let now = new Date ().getTime ();

    this.pause_time = now;

    clearInterval (this.interval);
    this.interval = null;
  }

  stop ()
  {
    this.pause ();

    this.start_time = null;
    this.refresh (this.duration*1000);
    this.control_panel.style.display = 'block';
  }

  static onTic (what)
  {
    let now       = new Date ().getTime ();
    let elapsed   = now - what.start_time;
    let remaining = what.duration*1000 - elapsed;

    if (remaining <= 0)
    {
      what.stop ();
    }
    else
    {
      what.refresh (remaining);
    }
  }

  onClicked ()
  {
    if (this.interval == null)
    {
      this.start ();
    }
    else
    {
      this.pause ();
    }
  }

  static onDurationSelected (what,
                             element)
  {
    what.duration = element.id.replace ('timekeeper-', '');
    what.display.style.backgroundColor = element.style.backgroundColor;
    what.stop ();
  }
};
