var speed=2
var currentpos=0,alt=1,curpos1=0,curpos2=-1, oldpos=0, pause=1, waitBeforeChange=0

function initialize(){startit()}

function scrollwindow()
{
	if (document.all && !document.getElementById)
		temp=document.body.scrollTop
	else
		temp=window.pageYOffset
	if (alt==0)
		alt=2
	else
		alt=1
		
	if (alt==0)
		curpos1=temp
	else
		curpos2=temp
		
	if (pause == 0)
	{
		if( waitBeforeChange > 0 )
			waitBeforeChange--
		else
		{
			if (document.all)
				currentpos=document.body.scrollTop+speed
			else
				currentpos=window.pageYOffset+speed
			
			if( currentpos == oldpos )
			{
				speed = speed * (-1)
				waitBeforeChange = 30
			}
			else
			{
				window.scroll(0,currentpos)
				oldpos = currentpos
			}
		}
	}
}

function startit(){setInterval("scrollwindow()",50)}

function switchPause()
{
	var div = document.getElementById( "defileButton" );
	if( pause == 0 )
	{
		pause = 1
		div.innerHTML = "Lancer défillement"
	}
	else
	{
		pause = 0
		div.innerHTML = "Stoper défillement"
	}
}

window.onload=initialize




function switchDisplay( idElement )
{
	if(document.getElementById( idElement ).style.display=='none')
	 	 document.getElementById( idElement ).style.display='block';
	else
	 	 document.getElementById( idElement ).style.display='none';
}