class QrCode extends Module
{
  constructor (hook,
               mode,
               piste_number)
  {
    super ('QrCode',
           hook);

    {
      let html = '';

      if (piste_number != null)
      {
        html += '<div id="piste_number">Piste ' + piste_number + '</div>';
      }
      html += '<div id="qrcode-text" class="qrcode-child"></div>';
      html += '<img id="qrcode-image" class="qrcode-child">';

      this.container.innerHTML = html;
    }
    this.img  = document.getElementById ('qrcode-image');
    this.text = document.getElementById ('qrcode-text');
  }

  setUrl (url)
  {
    let base64 = Message.getField (url, 'Body', 'qrcode.png');
    let text   = Message.getField (url, 'Body', 'qrcode.txt');

    this.img.src = 'data:image/png;base64,' + base64;
    this.text.innerHTML = text;
  }

  show ()
  {
    if (mode == 'mirror')
    {
      super.show ();
    }
  }

  lock ()
  {
    this.img.style.display  = 'none';
    this.text.style.display = 'none';
  }

  unlock ()
  {
    this.img.style.display  = 'inline-block';
    this.text.style.display = 'inline-block';
  }
}
