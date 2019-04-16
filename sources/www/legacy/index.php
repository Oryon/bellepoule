<?php
session_start();
require_once( "tools.php" );
require_once( "functions.php" );

$_SESSION[ 'cotcotFiles' ] = explorer( 'cotcot/' );
fillSessionWithTitreLong();
?>

<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.1//EN" "http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="fr">
	<head>
		<title>Compétition - Affichage</title>
		<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
		<link rel="stylesheet" media="screen" type="text/css" title="Design de base" href="index.css" />
		<link rel="SHORTCUT ICON" href="images/icone.gif" />
		<script language="javascript" src="functions.js" type="text/javascript"></script>
	</head>
	
	<body>
		
		<?php
			echo renderCompetitionTabControl();
			
			echo '
			<div class="competitionTabPage">';

			if( selectedCompetition() > -1 )
			{			
				$xml = new DOMDocument( "1.0", "utf-8" );
				$xml->load( $_SESSION[ 'cotcotFiles' ][ selectedCompetition() ] );

				echo renderStepTabControl( $xml );
				
				echo '
				<div class="stepTabPage">';
					echo renderCompetitionCartouche( $xml );
					
					switch( selectedPhaseName( $xml ) )
					{
						case 'ListeInscrit':
							echo renderTireurListPage( $xml );
							break;
						
						case 'TourDePoules' : 
							echo renderPoulePage( $xml );
							break;
						
						case 'PhaseDeTableaux' :
							echo renderTableau( $xml );
						break;
						
						case 'ClassementGeneral' :
							echo renderClassement( $xml );
						break;
						
						default:
						echo 'Phase non valable';
						break;
					}
				echo '
				</div>';
			}
			
			echo '
			</div>
			<div class="footer">Module créé par Guillaume Hayette - AS Caluire Escrime</div>';
		?>
	</body>
</html>
