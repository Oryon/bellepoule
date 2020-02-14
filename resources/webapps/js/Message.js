// -- Message ------------------
class Message {
  constructor (name,
               content = null)
  {
    if (name != null)
    {
      this.data = '';

      this.data += '[Header]\n';
      this.data += 'name=' + name + '\n';
      this.data += 'netid=0\n';
      this.data += 'fitness=1\n';
      this.data += '[Body]\n';
    }
    else
    {
      this.data = content;
    }
  }

  addField (name,
            value)
  {
    this.data += name + '=' + value + '\n';
  }

  static getField (message,
                   section_name,
                   field)
  {
    let section;
    let value = '';

    {
      let offset = message.indexOf ('[' + section_name + ']');

      section = message.slice (offset);
    }

    {
      let tag    = field + '=';
      let offset = section.indexOf (tag);

      if (offset != -1)
      {
        value = section.slice (offset + tag.length);
        value = value.split ('\n', 1)[0];
        value = value.replace (/\\n/g, '');
      }
    }

    return value;
  }
}
