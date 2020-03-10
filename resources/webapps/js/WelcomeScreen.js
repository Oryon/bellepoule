class WelcomeScreen extends Module
{
  constructor (hook,
               mode)
  {
    super ('WelcomeScreen',
           document.createElement ('img'));

    hook.appendChild (this.container);

    this.container.id  = this.name;
    this.container.src = 'BellePoule.png';
  }
}
