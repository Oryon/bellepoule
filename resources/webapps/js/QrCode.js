class QrCode extends Module
{
  constructor (hook,
               mode)
  {
    super ('QrCode',
           document.createElement ('div'));

    hook.appendChild (this.container);
    this.hide ();

    this.img = document.createElement ('img');
    this.container.appendChild (this.img);
    this.img.style.cssFloat = 'right';

    this.text = document.createElement ('div');
    this.container.appendChild (this.text);
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
