class QrCode extends Module
{
  constructor (hook,
               mode)
  {
    super ('QrCode',
           hook);

    this.hide ();

    {
      let html = '';

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
}
