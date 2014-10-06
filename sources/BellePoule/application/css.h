"    <style type=\"text/css\">\n"
"      body\n"
"      {\n"
"        background:  #434343;\n"
"        color:       #8ce566;\n"
"        font-family: Verdana, Geneva, Arial, Helvetica, sans-serif;\n"
"      }\n"
"      .Title h1\n"
"      {\n"
"        margin:       5;\n"
"      }\n"
"      .Round\n"
"      {\n"
"        font-style:  normal;\n"
"        font-weight: bold;\n"
"        display:     none;\n"
"      }\n"
"      .Table th\n"
"      {\n"
"        padding:     5px 5px 5px 5px;\n"
"      }\n"
"      .Table td\n"
"      {\n"
"        padding:     0px 5px 0px 5px;\n"
"        border:      20px;\n"
"      }\n"
"      .TableHeader\n"
"      {\n"
"        background:  #f58700;\n"
"        color:       Black;\n"
"      }\n"
"      .EvenRow\n"
"      {\n"
"        background:  #777777;\n"
"        color:       Black;\n"
"      }\n"
"      .OddRow\n"
"      {\n"
"        background:  #878787;\n"
"        color:       Black;\n"
"      }\n"
"      .Separator\n"
"      {\n"
"        outline: 1px solid #f58700;\n"
"      }\n"
"      .Footer\n"
"      {\n"
"        color:       White;\n"
"      }\n"
"      #menu_bar {\n"
"        list-style-type: none;\n"
"        padding-bottom:  28px;\n"
"        border-bottom:   1px solid #9EA0A1;\n"
"        margin-left:     0;\n"
"      }\n"
"      #menu_bar li {\n"
"        float:  left;\n"
"        height: 25px;\n"
"        margin: 2px 2px 0 4px;\n"
"        background:  #434343;\n"
"        border: 1px solid #9EA0A1;\n"
"      }\n"
"      #menu_bar li.active {\n"
"      }\n"
"      #menu_bar a {\n"
"        display : block;\n"
"        text-decoration : none;\n"
"        padding : 4px;\n"
"      }\n"
"      #menu_bar a:hover {\n"
"        background:  #f58700;\n"
"        color:       black;\n"
"      }\n"
"      A:link    {text-decoration: underline; color: #8ce566}\n"
"      A:visited {text-decoration: underline; color: #8ce566}\n"
"    </style>\n"
"\n"
"    <script language=\"JavaScript\">\n"
"      var current_selection;\n"
"\n"
"      // ----------------------------------------------\n"
"      function OnTabClicked (tab_id)\n"
"      {\n"
"        var element = document.getElementById (tab_id);\n"
"\n"
"        if (current_selection == null)\n"
"        {\n"
"          current_selection = document.getElementById (\'round_0\');\n"
"        }\n"
"        current_selection.style.display = \"none\";\n"
"\n"
"        current_selection = element;\n"
"        current_selection.style.display = \"inline\";\n"
"      }\n"
"    </script>\n"
