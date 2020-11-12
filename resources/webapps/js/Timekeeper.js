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
    if (mode != 'mirror')
    {
      this.listener.onClicked ();
    }
  }

  onSensorLongPress (sensor)
  {
    this.listener.stop ();
  }
}

class Timekeeper extends Module
{
  constructor (container, mode)
  {
    super ('Timekeeper',
           container);

    this.duration        = 3*60*1000;
    this.interval        = null;
    this.mirror_wait_tic = 0;
    this.start_time      = null;

    {
      let html = '';

      if (mode != 'mirror')
      {
        html += '<div id="timekeeper-control" class="timekeeper-control">'
      }
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

    this.durations = [{'seconds':3*60, 'color':'#607D3B'},
                      {'seconds':30,   'color':'#c6be00'}];

    if (this.control_panel)
    {
      this.addDurations (this.control_panel);
    }

    this.sensor = new TimekeeperSensor (this,
                                        document.getElementById ('timekeeper-display'));

    this.refresh (this.duration);
  }

  addDurations (control_panel)
  {
    let caller = this;
    let html   = '';

    for (let i = 0; i < this.durations.length; i++)
    {
      let seconds = this.durations[i].seconds;
      let color   = this.durations[i].color;

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

    for (let i = 0; i < this.durations.length; i++)
    {
      let seconds = this.durations[i].seconds;
      let element = document.getElementById ('timekeeper-' + seconds);

      element.onclick = function () {caller.onDurationSelected (element);};
    }
  }

  refresh (remaining)
  {
    let date     = new Date (remaining);
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

    if ((this.mirror_wait_tic == 0) && (mirror != ''))
    {
      let msg = new Message ('SmartPoule::Timer');

      msg.addField ('mirror',     mirror);
      msg.addField ('start_time', this.start_time);
      msg.addField ('duration',   this.duration);
      msg.addField ('remaining',  remaining);
      msg.addField ('running',    this.interval != null);

      socket.send (msg.data);
      this.mirror_wait_tic = 50;
    }
    else
    {
      this.mirror_wait_tic--;
    }
  }

  synchronize (start_time,
               duration,
               remaining,
               running)
  {
    let now = new Date ().getTime ();

    this.duration   = duration;
    this.start_time = now - duration + remaining;

    if (running == false)
    {
      this.pause ();
    }
    else if (this.interval == null)
    {
      let caller = this;

      this.interval = setInterval (function () {caller.onTic ();}, 100);
    }

    for (let i = 0; i < this.durations.length; i++)
    {
      if (this.durations[i].seconds == duration/1000)
      {
        this.display.style.backgroundColor = this.durations[i].color;
        break;
      }
    }
  }

  start ()
  {
    let caller = this;

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

    this.interval        = setInterval (function () {caller.onTic ();}, 100);
    this.mirror_wait_tic = 0;

    if (this.control_panel)
    {
      this.control_panel.style.display = 'none';
    }

    this.onTic ();
  }

  pause ()
  {
    let now = new Date ().getTime ();

    this.pause_time = now;

    clearInterval (this.interval);
    this.interval = null;

    this.mirror_wait_tic = 0;
    this.onTic ();
  }

  stop ()
  {
    this.pause ();

    this.start_time = null;
    this.mirror_wait_tic = 0;
    this.refresh (this.duration);

    if (this.control_panel)
    {
      this.control_panel.style.display = 'block';
    }
  }

  onTic ()
  {
    let now       = new Date ().getTime ();
    let elapsed   = now - this.start_time;
    let remaining = this.duration - elapsed;

    if (remaining <= 0)
    {
      this.stop ();
    }
    else
    {
      this.refresh (remaining);
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

  onDurationSelected (element)
  {
    this.duration = element.id.replace ('timekeeper-', '') * 1000;
    this.display.style.backgroundColor = element.style.backgroundColor;

    this.mirror_wait_tic = 0;
    this.refresh (this.duration);
  }
};
