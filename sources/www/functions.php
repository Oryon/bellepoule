<?php

function renderCompetitionTabControl()
{
	$colorList = array("#F781F3","#BE81F7","#8181F7","#81BEF7","#81F7F3","#81F7BE","#81F781","#BEF781","#F3F781");

	$tab = '
	<div class="competitionTabControl" id="competitionTabControl">';
	
		$k = 0;
		foreach( $_SESSION[ 'titreList' ] as $titre )
		{
			$color = $colorList[ $k ];
			$class = 'competitionTab';
			if( selectedCompetition() == $k )
				$class .= 'Active';
			
			$tab .= '
			<div class="' . $class . '" style="background-color: ' . $color . ';"><a href="' . makeUrl( array( 'idCompetition' => $k ) ) . '">' . $titre . '</a></div>';
			
			$k++;
		}
			
		$tab .= '
		<div class="competitionTabRefresh"><a href="' . makeUrl( array( 'refresh' => '1' ) ) . '">Rafraichir</a></div>';
	
	$tab .= '
	</div>';
	
	$tab .= '
	<div class="defileButton" id="defileButton" onClick="switchDisplay( \'competitionTabControl\' );switchDisplay( \'stepTabControl\' );switchPause()">Lancer défillement</div>
	<div class="settingButton" id="settingButton" onClick="switchDisplay( \'settingForm\' );">Settings</div>';
	
	return $tab;
}

function renderSettingForm( $selectedPhaseXml )
{
	$form = '';
	
	$form .= '
	<div class="settingForm" id="settingForm">
		<div class="closeButton" onclick="switchDisplay( \'settingForm\' );"></div>
		<div class="form">';
		
			if( $selectedPhaseXml->localName == 'PhaseDeTableaux' )
			{
				$tabStart = 1;
				$tabEnd = 1;
				if( isset( $_POST[ 'tabStart' ] ) )
				{
					$tabStart = $_POST[ 'tabStart' ];
					$tabEnd = $_POST[ 'tabEnd' ];
				}
				else
				{				
					$suiteId = '0';
					if( isset( $_REQUEST[ 'suiteId' ] ) )
						$suiteId = $_REQUEST[ 'suiteId' ];
					
					$suiteXml = getSuiteById( $selectedPhaseXml, $suiteId );
					foreach( $suiteXml->childNodes as $tableauXml )
					{
						if( $tableauXml->localName == 'Tableau' )
						{
							$tabStart = substr( getAttribut( $tableauXml, 'ID' ), 1, strlen( getAttribut( $tableauXml, 'ID' ) ) - 1 );
							break;
						}
					}
				}
				
				$form .= '
				<form method="post" action="">
					Afficher du tablau de
					<select name="tabStart">';
						for( $i = $tabStart; $i >= 2; $i /= 2 )
						{
							$selected = '';
							if( $tabStart == $i )
								$selected = 'selected';
							
							$form .= '
							<option ' . $selected . '>' . $i . '</option>';
						}
					$form .='
					</select>
					<br/>
					au tableau de 
					<select name="tabEnd">';
						for( $i = $tabStart; $i >= 1; $i/= 2 )
						{
							$selected = '';
							if( $tabEnd == $i )
								$selected = 'selected';
							
							$form .= '
							<option ' . $selected . '>' . $i . '</option>';
						}
					$form .='
					</select>
					<br/>
					<input type="submit" value="Valider" />
				</form>';
			}
			else
				$form .= 'Aucune option disponible pour cette phase de la compétition';
				
		$form .= '
		</div>
	</div>';

	return $form;
}

function renderCompetitionCartouche( $docXml )
{
	$cartouche = '';
	
	$cartouche .= '
	<div class="cartouche" id="cartouche">
		' . getAttribut( $docXml->documentElement, 'TitreLong' ) . '<br/>
		Compétition ' . getCategorieLibelle( getAttribut( $docXml->documentElement, 'Categorie' ) ) . ' ' . getSexeLabel( getAttribut( $docXml->documentElement, 'Sexe' ) ) . '
	</div>';
	
	return $cartouche;
}


/// En dur, à faire avec l'xml
function renderStepTabControl( $docXml )
{
	$tab = '';
	
	$phaseId = -1;
	$class = 'stepTab';
	if( $phaseId == selectedPhase() )
		$class .= 'Active';
						
	$tab .= '
	<div class="stepTabControl" id="stepTabControl">
		<div class="' . $class . '"><a href="' . makeUrl( array( 'phaseId' => -1 ) ) . '">Liste des tireurs inscrits</a></div>';
	
		$phasesXml = $docXml->getElementsByTagName( 'Phases' );
		
		foreach( $phasesXml as $phases ) 
		{
			foreach($phases->childNodes as $phase)
			{
				if( isset( $phase->tagName ) )
				{
					$phaseId = getAttribut( $phase, 'PhaseID' );
					$class = 'stepTab';
					if( $phaseId == selectedPhase() )
						$class .= 'Active';
					
					$tab .= '
					<div class="' . $class . '"><a href="' . makeUrl( array( 'phaseId' => getAttribut( $phase, 'PhaseID' ) ) ) . '">' . $phase->tagName . '</a></div>';
				}
			}
		}
	
	$tab .= '
	</div>';
	
	return $tab;
}


/********************************************************/

/*                   LISTE DES INSCRITS                 */

/********************************************************/
function renderTireurListPage( $domXml )
{
	$list = '';

	$equipesListXml	= $domXml->getElementsByTagName( 'Equipes' );
	foreach( $equipesListXml as $equipesXml ) 
	{
		$equipeCount = getTireurCount( $equipesXml );
		if( $equipeCount > 0 )
		{
			$pair = "pair";
			
			$list .= '
			<table class="listeTireur">
				<tr class="pair">
					<td colspan="2">' . $equipeCount . ' équipes présentes</td>
				</tr>
				<tr>
					<th>Equipe</th>
					<th>Club</th>
					<th>Classement</th>
				</tr>';
				
				foreach( $equipesXml->childNodes as $equipeXml )
				{
					if( $equipeXml->localName == 'Equipe' && getAttribut( $equipeXml, 'Statut' ) == STATUT_PRESENT )
					{
						$list .= '
						<tr class="equipe">
							<td>' . getAttribut( $equipeXml, 'Nom' ) . '</td>
							<td>' . getAttribut( $equipeXml, 'Club' ) . '</td>
							<td>' . getAttribut( $equipeXml, 'Ranking' ) . '</td>
						</tr>';
						
						$pair = "pair";
						
						foreach( $equipeXml->childNodes as $tireurXml )
						{
							if( $tireurXml->localName == 'Tireur' )
							{
								$list .= '
								<tr class="' . $pair . '">
									<td class="tireur"><t/>' . getAttribut( $tireurXml, 'Nom' ) . ' ' . getAttribut( $tireurXml, 'Prenom' ) . '</td>
									<td><t/>' . getAttribut( $tireurXml, 'Club' ) . '</td>
									<td><t/>' . getAttribut( $tireurXml, 'Ranking' ) . '</td>
								</tr>';
						
								$pair = $pair == "pair" ? "impair" : "pair";
							}
						}
					}
				}
			$list .= '
			</table>';
		}
	}
	
	$tireursXml	= $domXml->getElementsByTagName( 'Tireurs' );
	foreach ($tireursXml as $tireurs) 
	{
		$tireurCount = getTireurCount( $tireurs );
		if( $tireurCount > 0 )
		{
			$tireurLabel = ' tireurs présents';
			if( $equipeCount > 0 )
				$tireurLabel = ' tireurs sans équipe';
				
			$list .= '
			<table class="listeTireur">
				<tr class="pair">
					<td colspan="2">' . $tireurCount . $tireurLabel . '</td>
				</tr>
				<tr>
					<th>Rang</th>
					<th>Nom</th>
					<th>Prénom</th>
					<th>Sexe</th>
					<th>Club</th>
					<th>Ligue</th>
				</tr>';
				
				$pair = "pair";
				foreach( $tireurs->childNodes as $tireur )
				
				// $tireurXml = $tireurs->getElementsByTagName( 'Tireur' );
				// foreach ($tireurXml as $tireur) 
				{
					if( $tireur->localName == 'Tireur' && getAttribut( $tireur, 'Statut' ) == STATUT_PRESENT )
					{
						$list .= '
						<tr class="'. $pair . '">
							<td>' . getAttribut( $tireur, 'Ranking' ) . '</td>
							<td>' . getAttribut( $tireur, 'Nom' ) . '</td>
							<td>' . getAttribut( $tireur, 'Prenom' ) . '</td>
							<td>' . getAttribut( $tireur, 'Sexe' ) . '</td>
							<td>' . getAttribut( $tireur, 'Club' ) . '</td>
							<td>' . getAttribut( $tireur, 'Ligue' ) . '</td>
						</tr>';
						
						$pair = $pair == "pair" ? "impair" : "pair";
					}
				}
					
			$list .= '
			</table>';
		}
	}
	
	$list .= renderFormule( $domXml, $tireurCount, $equipeCount );
	
	return $list;
}

function renderFormule( $domXml, $tireurCount, $equipeCount )
{
	$list = 
	'<div class="formule">
		<h2>Formule : </h2>';
		
		$femMark = '';
		if( getAttribut( $domXml->documentElement, 'Sexe' ) == SEXE_FEMALE )
			$femMark = 'e';

		if( $equipeCount > 0 )
		{
			$list .= '
			<p>' . $equipeCount . ' équipes présentes</p>';
		}
		else
		{
			$presentLabel = ' tireurs présents';
			if( getAttribut( $domXml->documentElement, 'Sexe' ) == SEXE_FEMALE )
				$presentLabel = ' tireuses présentes';
			
			$list .= '
			<p>' . $tireurCount . $presentLabel . '</p>';
		}

		$phasesXml = $domXml->getElementsByTagName( 'Phases' );
		foreach( $phasesXml as $phases ) 
		{
			foreach($phases->childNodes as $phase)
			{
				if( isset( $phase->tagName ) )
				{
					switch( $phase->tagName )
					{
						case 'TourDePoules' :
							$nbQualifie = getAttribut( $phase, 'NbQualifiesParIndice' );
							if( $nbQualifie == '' )
								$nbQualifie = 'Pas d\'éliminé' . $femMark;
							else
								$nbQualifie .= ' qualifié' . $femMark . 's';
							
							$list .= '
							<p>' . getAttribut( $phase, 'ID' ) . '<br/>' . 
							$nbQualifie . '</p>';
							break;
						
						case 'PhaseDeTableaux' :
							$list .= '
							<p>Tableau d\'élimination directe';
							
							switch( getAttribut( $phase, 'PlacesTirees' ) )
							{
								case 3:
									$list .= '<br/>3ème place tirée';
									break;
								
								case 99:
									$list .= '<br/>Toutes les places sont tirées';
									break;
								
								default:
									break;
							}
							
							$list.= '
							</p>';
							break;
						
						case 'ClassementGeneral' :
						case '' :
							break;

						default:
						echo 'Phase inconnue : ' . $phase->tagName;
						break;
					}
				}
			}
		}
		
	$list .= '
	</div>';
	
	return $list;
}

function getTireurCount( $tireursXml )
{
	$tireurCount = 0;
	
	foreach( $tireursXml->childNodes as $tireurXml )
	{
		if( get_class( $tireurXml ) == 'DOMElement' && getAttribut( $tireurXml, 'Statut' ) == STATUT_PRESENT )
			$tireurCount++;
	}
	
	return $tireurCount;
}

function getEquipeMaxLength( $equipesXml )
{
	$equipeLength = 0;
	
	foreach( $equipesXml->childNodes as $equipeXml )
	{
		$tireurCount = 0;
		
		if( get_class( $equipeXml ) == 'DOMElement' )
		{
			foreach( $equipeXml->childNodes as $tireurXml )
			{
				if( get_class( $tireurXml ) == 'DOMElement' && $tireurXml->localName == 'Tireur' )
					$tireurCount++;
			}
			
			if( $tireurCount > $equipeLength )
				$equipeLength = $tireurCount;
		}
	}
	
	return $equipeLength;
}

/********************************************************/

/*                         POULES                       */

/********************************************************/

define( 'RANK_INIT', 0 );
define( 'NO_IN_POULE', 1 );
define( 'NB_VICT', 2 );
define( 'NB_MATCH', 3 );
define( 'TD', 4 );
define( 'TR', 5 );
define( 'RANK_IN_POULE', 6 );
define( 'RANK_FIN', 7 );
define( 'STATUT', 8 );
define( 'FIRST_RES', 9 );

function renderPoulePage( $domXml )
{
	$poules = '';
	
	$phase = getPhaseXmlElement( $domXml, $_GET[ 'phaseId' ] );
	if( $phase != null )
	{
		$poules .= '
		<div class="PouleTabControl" >
			<div class="PouleTab"><a href="' . makeUrl( array( "page" => "poules" ) ) . '">Poules</a></div>
			<div class="PouleTab"><a href="' . makeUrl( array( "page" => "classement" ) ) . '">Classement</a></div>
		</div>';

		if( isset( $_GET[ 'page' ] ) )
		{
			switch( $_GET[ 'page' ] )
			{
				case "classement":
					$poules .= renderPouleClassement( $domXml, $phase );
					break;
				case "poules":
				default:
					$poules .= renderPouleList( $domXml, $phase );
					break;
			}
		}
		else
			$poules .= renderPouleList( $domXml, $phase );
	}
	
	return $poules;
}

function renderPouleList( $domXml, $phase )
{
	$poules = '';
	
	$tireurList = getTireurList( $domXml );
	$tireurRanking = getTireurRankingList( $phase );

	$pouleListXml = $phase->getElementsByTagName( 'Poule' );
	
	//*** nbMaxTireur
	$nbMaxTireur = 0;
	foreach ($pouleListXml as $pouleXml) 
	{
		$tireurCount = getPouleTireurCount( $pouleXml );
		
		if( $nbMaxTireur < $tireurCount )
			$nbMaxTireur = $tireurCount;
	}
	
	foreach ($pouleListXml as $pouleXml) 
	{
		$tireurCount = getPouleTireurCount( $pouleXml );
		
		$poules .= '
			<table class="poule">
				<tr>
					<th colspan="2" class="pouleNumber">Poule ' . getAttribut( $pouleXml, 'ID' ) . ' - Piste <input type="text" size="2" value="' . getAttribut( $pouleXml, 'ID' ) . '" /></th>
					<th class="number"></th>';
					
					for( $i = 1; $i <= $tireurCount; $i++ )
					{
						$poules .= '
						<th class="number">' . $i . '</th>';
					}
					
					$poules .= '
					<th class="victoire">V</th>
					<th class="number">V/M</th>
					<th class="number">TD</th>
					<th class="number">TR</th>
					<th class="number">I</th>
					<th class="number">Clt</th>
				</tr>';
				
				$cpt = 1;
				foreach( $pouleXml->childNodes as $pouleChild )
				{
					if( isset( $pouleChild->tagName ) )
					{
						$refTireur = getAttribut( $pouleChild, 'REF' );
						if( isset( $tireurList[ $refTireur ] ) )
						{
							$initialRanking = "";
							$noInPoule = "";
							if( isset( $tireurRanking[ $refTireur ] ) )
							{
								$initialRanking = $tireurRanking[ $refTireur ][ RANK_INIT ];
								$noInPoule = $tireurRanking[ $refTireur ][ NO_IN_POULE ];
							}
							$poules .= '
							<tr class="tireurRow">
								<td class="tireurClassement">' . $initialRanking . '</td>
								<td class="tireurName">' . getAttribut( $tireurList[ $refTireur ], 'Nom' ) . ' ' . getAttribut( $tireurList[ $refTireur ], 'Prenom' ) . '</td>
								<td class="number">' . $noInPoule . '</td>';
								
								for( $i = 1; $i <= $tireurCount; $i++ )
								{
									$class = "inside";
									if( $i == $noInPoule )
										$class .= 'black';
									
									$matchRes = '';
									if( isset( $tireurRanking[ $refTireur ][ FIRST_RES + $i - 1 ] ) )
										$matchRes = $tireurRanking[ $refTireur ][ FIRST_RES + $i - 1 ];
									
									if( $matchRes == POULE_STATUT_ABANDON )
									{
										$matchRes = 'Ab';
										$class .= 'Abandon';
									}
									if( $matchRes == POULE_STATUT_EXPULSION )
									{
										$matchRes = 'Ex';
										$class .= 'Abandon';
									}
									
									$poules .= '
									<td class="' . $class . '">' . $matchRes . '</td>';
								}
							
								$indice = $tireurRanking[ $refTireur ][ TD ] - $tireurRanking[ $refTireur ][ TR ];
								$poules .= '
								<td class="victoire">' . $tireurRanking[ $refTireur ][ NB_VICT ] . '</td>
								<td class="inside">' . round( $tireurRanking[ $refTireur ][ NB_VICT ] / $tireurRanking[ $refTireur ][ NB_MATCH ], 2 ) . '</td>
								<td class="inside">' . $tireurRanking[ $refTireur ][ TD ] . '</td>
								<td class="inside">' . $tireurRanking[ $refTireur ][ TR ] . '</td>
								<td class="inside">' . $indice . '</td>
								<td class="inside">' . $tireurRanking[ $refTireur ][ RANK_IN_POULE ] . '</td>
							</tr>';
						}
					
						if( $cpt == $tireurCount )
						{
							while( $cpt++ < $nbMaxTireur )
							{
								$poules .= '
								<tr class="complement"><td></td><tr>';
							}
						}
						
						$cpt++;
					}
				}
			
			$poules .= '
			</table>';
	}
	
	// tableau supplémentaire pour gérer le parent de tous les table float
	$poules .= '
	<table style="clear: both"></table>';
	
	return $poules;
}

function renderPouleClassement( $domXml, $phase )
{
	$tireurList    = getTireurList( $domXml );
	$tireurRanking = getTireurRankingList( $phase );

	$class = '
	<table class="listeTireur">
		<tr>
			<td colspan="4">Classement à l\'issue du ' . getAttribut( $phase, 'ID' ) . '</td>
		</tr>
		<tr>
			<th>Rang</th>
			<th>Nom</th>
			<th>Prénom</th>
			<th>Club</th>
			<th>V / M</th>
			<th>Ind</th>
			<th>TD</th>
			<th>Statut</th>
		</tr>';

		$currentRank = 0;
		$pair = "pair";
		while( ++$currentRank <= count( $tireurRanking ) )
		{
			foreach( $tireurRanking as $id => $tireur )
			{
				if( isset(  $tireur[ RANK_FIN ] ) && $tireur[ RANK_FIN ] == $currentRank )
				{
					$indice = $tireur[ TD ] - $tireur[ TR ];
					$statut = '' ;
					switch( $tireur[ STATUT ] )
					{
						case STATUT_PRESENT :
							$statut = 'Qualifié';
							break;
						case STATUT_ELIMINE: 
							$statut = 'Eliminé';
							break;
						case POULE_STATUT_ABANDON: 
							$statut = 'Abandon';
							break;
						case POULE_STATUT_EXPULSION: 
							$statut = 'Expulsion';
							break;
					}
					
					$class .= '
					<tr class="' . $pair . '">
						<td>' . $tireur[ RANK_FIN ] . '</td>
						<td>' . getAttribut( $tireurList[ $id ], 'Nom' ) . '</td>
						<td>' . getAttribut( $tireurList[ $id ], 'Prenom' ) . '</td>
						<td>' . getAttribut( $tireurList[ $id ], 'Club' ) . '</td>
						<td>' . round( $tireur[ NB_VICT ] / $tireur[ NB_MATCH ], 2 ) . '</td>
						<td>' . $indice. '</td>
						<td>' . $tireur[ TD ] . '</td>
						<td>' . $statut . '</td>
					</tr>';
					
					$pair = $pair == "pair" ? "impair" : "pair";
				}
			}
		}
		
	$class .= '
	</table>';
	
	return $class;
}

function getPouleTireurCount( $pouleXml )
{
	$tireurCount = 0;
	foreach( $pouleXml->childNodes as $pouleChild )
	{
		if( get_class( $pouleChild ) == 'DOMElement' && ( $pouleChild->localName == 'Tireur' || $pouleChild->localName == 'Equipe' ) )
			$tireurCount++;
	}
	
	return $tireurCount;
}

function getTireurList( $domXml )
{
	$tireurList = array();
	
	$equipesListXml = $domXml->getElementsByTagName( 'Equipes' );
	foreach( $equipesListXml as $equipesXml ) 
	{
		foreach( $equipesXml->childNodes as $equipeXml ) 
		{
			if( get_class( $equipeXml ) == 'DOMElement' )
				$tireurList[ getAttribut( $equipeXml, 'ID' ) ] = $equipeXml;
		}
	}
	
	if( count( $tireurList ) == 0 )
	{
		$tireursXml	= $domXml->getElementsByTagName( 'Tireurs' );
		foreach ($tireursXml as $tireurs) 
		{
			$tireurXml = $tireurs->getElementsByTagName( 'Tireur' );
			foreach ($tireurXml as $tireur) 
			{
				$tireurList[ getAttribut( $tireur, 'ID' ) ] = $tireur;
			}
		}
	}
	return $tireurList;
}
	
function getTireurRankingList( $phaseXml )
{
	$tireurList = array();
	
	foreach( $phaseXml->childNodes as $phaseChild )
	{
		if( isset( $phaseChild->localName ) )
		{
			if( $phaseChild->localName == 'Tireur' || $phaseChild->localName == 'Equipe' )
			{
				$tireurList[ getAttribut( $phaseChild, 'REF' ) ] = array_fill( 0, 15, "" );
				$tireurList[ getAttribut( $phaseChild, 'REF' ) ][ RANK_INIT ] = getAttribut( $phaseChild, 'RangInitial' );
				$tireurList[ getAttribut( $phaseChild, 'REF' ) ][ RANK_FIN ] = getAttribut( $phaseChild, 'RangFinal' );
				$tireurList[ getAttribut( $phaseChild, 'REF' ) ][ STATUT ] = getAttribut( $phaseChild, 'Statut' );
			}
			else if( $phaseChild->localName == 'Poule' )
			{
				foreach( $phaseChild->childNodes as $pouleChild )
				{
					if( isset( $pouleChild->localName ) )
					{
						$tireurList[ getAttribut( $pouleChild, 'REF' ) ][ NO_IN_POULE ] = getAttribut( $pouleChild, 'NoDansLaPoule' );
						$tireurList[ getAttribut( $pouleChild, 'REF' ) ][ NB_VICT ] = getAttribut( $pouleChild, 'NbVictoires' );
						$tireurList[ getAttribut( $pouleChild, 'REF' ) ][ NB_MATCH ] = getAttribut( $pouleChild, 'NbMatches' );
						$tireurList[ getAttribut( $pouleChild, 'REF' ) ][ TD ] = getAttribut( $pouleChild, 'TD' );
						$tireurList[ getAttribut( $pouleChild, 'REF' ) ][ TR ] = getAttribut( $pouleChild, 'TR' );
						$tireurList[ getAttribut( $pouleChild, 'REF' ) ][ RANK_IN_POULE ] = getAttribut( $pouleChild, 'RangPoule' );
					}
				}
			}
		}
	}
	
	$matchXml = $phaseXml->getElementsByTagName( 'Match' );
	foreach( $matchXml as $match ) 
	{
		//*** 2 tireurs par match
		$tireur1Ref = -1;
		$tireur1Pos = -1;
		$tireur1Mark = -1;
		$tireur2Ref = -1;
		$tireur2Pos = -1;
		$tireur2Mark = -1;
		
		$k = 1;
		foreach( $match->childNodes as $tireur )
		{
			if( isset( $tireur->tagName ) )
			{
				if( $k == 1 )
				{
					$tireur1Ref = getAttribut( $tireur, 'REF' );
					$tireur1Pos = $tireurList[ $tireur1Ref ][ NO_IN_POULE ];
					if( getAttribut( $tireur, 'Statut' ) == POULE_STATUT_VICTOIRE )
					{
						$tireur1Mark = getAttribut( $tireur, 'Statut' );
						if( getAttribut( $tireur, 'Score' ) != getAttribut( $phaseXml, 'ScoreMax' ) )
							$tireur1Mark .= getAttribut( $tireur, 'Score' );
					}
					else if( getAttribut( $tireur, 'Statut' ) == POULE_STATUT_ABANDON )
					{
						$tireur1Mark = POULE_STATUT_ABANDON;
						$tireur2Mark = POULE_STATUT_ABANDON;
					}
					else if( getAttribut( $tireur, 'Statut' ) == POULE_STATUT_EXPULSION )
					{
						$tireur1Mark = POULE_STATUT_EXPULSION;
						$tireur2Mark = POULE_STATUT_EXPULSION;
					}
					else
						$tireur1Mark = getAttribut( $tireur, 'Score' );
				}
				else
				{
					$tireur2Ref = getAttribut( $tireur, 'REF' );
					$tireur2Pos = $tireurList[ $tireur2Ref ][ NO_IN_POULE ];
					if( getAttribut( $tireur, 'Statut' ) == POULE_STATUT_VICTOIRE )
					{
						$tireur2Mark = getAttribut( $tireur, 'Statut' );
						if( getAttribut( $tireur, 'Score' ) != getAttribut( $phaseXml, 'ScoreMax' ) )
							$tireur2Mark .= getAttribut( $tireur, 'Score' );
					}
					else if( getAttribut( $tireur, 'Statut' ) == POULE_STATUT_ABANDON )
					{	
						$tireur1Mark = POULE_STATUT_ABANDON;
						$tireur2Mark = POULE_STATUT_ABANDON;
					}
					else if( getAttribut( $tireur, 'Statut' ) == POULE_STATUT_EXPULSION )
					{	
						$tireur1Mark = POULE_STATUT_EXPULSION;
						$tireur2Mark = POULE_STATUT_EXPULSION;
					}
					else if( $tireur2Mark != POULE_STATUT_ABANDON && $tireur2Mark != POULE_STATUT_EXPULSION )
						$tireur2Mark = getAttribut( $tireur, 'Score' );
				}
				
				$k++;
			}
		}
		
		$tireurList[ $tireur1Ref ][ FIRST_RES + $tireur2Pos - 1 ] = $tireur1Mark;
		$tireurList[ $tireur2Ref ][ FIRST_RES + $tireur1Pos - 1 ] = $tireur2Mark;
		
		//echo $tireur1Ref . ' Vs ' . $tireur2Ref . ' -> ' . $tireur1Mark . ' (' . $tireur2Pos . ') ' . ' / ' . $tireur2Mark . ' (' . $tireur1Pos . ')<br/> ';
	}
	
	return $tireurList;
}



/********************************************************/

/*                         TABLEAU                      */

/********************************************************/
function renderTableau( $domXml )
{
	$tab = '';
	
	$t128 = array ( '1','','128','','65','','64','','33','','96','','97','','32','','17','','112','','81','','48','','49','','80','','113','','16','','9','','120','','73','','56','','41','','88','','105','','24','','25','','104','','89','','40','','57','','72','','121','','8','','5','','124','','69','','60','','37','','92','','101','','28','','21','','108','','85','','44','','53','','76','','117','','12','','13','','116','','77','','52','','45','','84','','109','','20','','29','','100','','93','','36','','61','','68','','125','','4','','3','','126','','67','','62','','35','','94','','99','','30','','19','','110','','83','','46','','51','','78','','115','','14','','11','','118','','75','','54','','43','','86','','107','','22','','27','','102','','91','','38','','59','','70','','123','','6','','7','','122','','71','','58','','39','','90','','103','','26','','23','','106','','87','','42','','55','','74','','119','','10','','15','','114','','79','','50','','47','','82','','111','','18','','31','','98','','95','','34','','63','','66','','127','','2');
	
	$phaseXml = getPhaseXmlElement( $domXml, $_GET[ 'phaseId' ] );
	if( $phaseXml != null )
	{
	/*	$startTabCount = 1;
		$startPowCount = 1;
		$startPowCountRequest = 1;
		$tireurCount = getTireurCount( $phaseXml );
		
		for( $startPowCount = 1; $startPowCount < 15; $startPowCount++ )
		{
			if( pow( 2, $startPowCount ) > $tireurCount )
			{
				break;
			}
		}
		
		$startPowCountRequest = $startPowCount;
		if( isset( $_POST[ 'tabStart' ] ) )
		{
			$startTabCount = $_POST[ 'tabStart' ];
			$startPowCountRequest = ( log( $startTabCount ) ) / ( log( 2 ) );
		}
		else
			$startTabCount = pow( 2, $startPowCount );*/
		
		echo renderSettingForm( $phaseXml );
		
		$idSuite = '0';
		if( isset( $_REQUEST[ 'suiteId' ] ) )
			$idSuite = $_REQUEST[ 'suiteId' ];
		foreach( $phaseXml->childNodes as $phaseChildXml )
		{
			if( get_class( $phaseChildXml ) == 'DOMElement' && $phaseChildXml->localName == 'SuiteDeTableaux' && getAttribut( $phaseChildXml, 'ID' ) == $idSuite )
			{
				$tab .= renderSuiteDeTableau( $domXml, $phaseXml, $phaseChildXml );
				break;
			}
		}
	
		return $tab;
	}
	
	return $tab;
}

function renderSuiteDeTableau( $domXml, $phaseXml, $suiteXml )
{
	$tab = '';
	
	$t128 = array ( '1','','128','','65','','64','','33','','96','','97','','32','','17','','112','','81','','48','','49','','80','','113','','16','','9','','120','','73','','56','','41','','88','','105','','24','','25','','104','','89','','40','','57','','72','','121','','8','','5','','124','','69','','60','','37','','92','','101','','28','','21','','108','','85','','44','','53','','76','','117','','12','','13','','116','','77','','52','','45','','84','','109','','20','','29','','100','','93','','36','','61','','68','','125','','4','','3','','126','','67','','62','','35','','94','','99','','30','','19','','110','','83','','46','','51','','78','','115','','14','','11','','118','','75','','54','','43','','86','','107','','22','','27','','102','','91','','38','','59','','70','','123','','6','','7','','122','','71','','58','','39','','90','','103','','26','','23','','106','','87','','42','','55','','74','','119','','10','','15','','114','','79','','50','','47','','82','','111','','18','','31','','98','','95','','34','','63','','66','','127','','2');
	
	$placeTableau = substr( getAttribut( $suiteXml, 'Titre' ), 8, strlen( getAttribut( $suiteXml, 'Titre' ) ) - 1 );
	foreach( $suiteXml->childNodes as $tableauXml )
	{
		if( get_class( $tableauXml ) == 'DOMElement' && $tableauXml->localName == 'Tableau' )
		{
			$noTab = substr( getAttribut( $tableauXml, 'ID' ), 1, strlen( getAttribut( $tableauXml, 'ID' ) ) - 1 );
			break;
		}
	}
	
	for( $startPowCount = 1; $startPowCount < 15; $startPowCount++ )
	{
		if( pow( 2, $startPowCount ) == $noTab )
			break;
	}
	
	$minInd = $placeTableau;
	$maxInd = $placeTableau + $noTab - 1;
	
	$tireurTab  = getTireurTab( $domXml, $phaseXml, $suiteXml, $startPowCount, $minInd, $maxInd );
	$tireurList = getTireurList( $domXml );
	
	$tab .= '
	<table class="tableau">';
		
		$tab .= renderTHeader( $suiteXml );
		
		$startTabCount = $noTab;
		$startPowCount = ( log( $startTabCount ) ) / ( log( 2 ) );
		$startPowCountRequest = $startPowCount;
		
		$startPowCountRequest = $startPowCount;
		if( isset( $_POST[ 'tabStart' ] ) )
		{
			$startTabCount = $_POST[ 'tabStart' ];
			$maxInd = $startTabCount;
			$startPowCountRequest = ( log( $startTabCount ) ) / ( log( 2 ) );
		}
		else
			$startTabCount = pow( 2, $startPowCount );
		
		$classTab = array_fill( 0, 15, "top" );
		$i = 1;
		for( $ind = 0; $ind < count( $t128 ); $ind++ )
		{
			$current = $t128[ $ind ];
			
			if( ( $current <= $maxInd && $current >= $minInd ) || $current == '' )
			{
				$tab .= '
				<tr>';
					$col = -1;
					$currentTab = $startTabCount;
					$currentPow = ( log( $currentTab ) ) / ( log( 2 ) );
					//$currentPow = $startPowCountRequest;
					$currentRg = $current;
					$maxTab = 1;
					if( isset( $_POST[ 'tabEnd' ] ) )
						$maxTab = $_POST[ 'tabEnd' ];
					
					while( $currentTab >= $maxTab )
					{
						$col ++;
						// if( $currentRg > $currentTab )
							// $currentRg = $currentTab * 2 + 1 - $currentRg;

						$pow = pow( 2, $col );
						
						if( ( $i + $pow - 1 ) % $pow == 0 )
						{
							$labelLine = '';
							
							//echo 'isset [ ' . $currentPow . ' ][ ' . $currentRg . ' ] ? <br/>';
							if( $classTab[ $col ] == "middle" || $classTab[ $col ] == "" )
								$labelLine = '';
							else if( isset( $tireurTab[ $currentPow ][ $currentRg ] ) && isset( $tireurTab[ $currentPow ][ $currentRg ][ 'refTireur' ] ) )
							{
								//echo ' ' . getAttribut( $tireurList[ $tireurTab[ $currentPow ][ $currentRg ][ 'refTireur' ] ], 'Nom' ) . '<br/>';
								if( $col == 0 )
									$labelLine  .= '<div class="tabRg">' . $currentRg . '</div>';
									
								$labelLine  .= '
									<div class="tabNom">' . getAttribut( $tireurList[ $tireurTab[ $currentPow ][ $currentRg ][ 'refTireur' ] ], 'Nom' ) . ' ' . 
									getAttribut( $tireurList[ $tireurTab[ $currentPow ][ $currentRg ][ 'refTireur' ] ], 'Prenom' ) . '</div>';
								if( isset( $tireurTab[ $currentPow ][ $currentRg ][ 'score' ] ) && $tireurTab[ $currentPow ][ $currentRg ][ 'statut' ] != POULE_STATUT_ABANDON && $tireurTab[ $currentPow ][ $currentRg ][ 'statut' ] != POULE_STATUT_EXPULSION )
								{
									$labelLine .= '
									<div class="tabScore">' . $tireurTab[ $currentPow ][ $currentRg ][ 'statut' ] . $tireurTab[ $currentPow ][ $currentRg ][ 'score' ] . '</div>';
								}
							}
							else
							{
								//if( $col == 0 )
									$labelLine  .= '<div class="tabRg">' . $currentRg . '</div>';
							}
							
							
							if( $currentTab > 1 || $classTab[ $col ] == "top" )
							{
								$tab .= 
								'<td rowspan="' . $pow . '" class="' . $classTab[ $col ] . '">' . $labelLine . '</td>';
							}
							
							if( $classTab[ $col ] == "top" )
								$classTab[ $col ] = "middle";
							else if( $classTab[ $col ] == "middle" )
								$classTab[ $col ] = "bottom";
							else if( $classTab[ $col ] == "bottom" )
								$classTab[ $col ] = "";
							else if( $classTab[ $col ] == "" )
								$classTab[ $col ] = "top";
							
						}
						
						$currentRg = min( $currentRg, getAdversaryRank( $currentTab, $currentRg, $minInd ) );
						
						$currentTab /= 2;
						$currentPow --;
					}
					
					$i++;
					
				$tab .= '
				</tr>';
			}
			else
			{
				//echo 'no<br/>';
				$ind++;
			}
		}
		
	$tab .= '
	</table>';
	return $tab;
}

function renderTHeader( $suiteXml )
{
	$head = '';
	
	$maxTab = 1;
	if( isset( $_POST[ 'tabEnd' ] ) )
		$maxTab = $_POST[ 'tabEnd' ];
	
	$tabTitle = 'Tableau principal';
	if( getAttribut( $suiteXml, 'ID' ) !== '0' )
		$tabTitle = getAttribut( $suiteXml, 'Titre' ) ;

	$head .= '
	<tr>
		<th class="tabTitle" colspan="2">' . $tabTitle . '</th>
	</tr>';
		
	$head .= '
	</tr>';
	
	foreach( $suiteXml->childNodes as $tableauXml )
	{
		if( get_class( $tableauXml ) == 'DOMElement' && $tableauXml->localName == 'Tableau' )
		{
			$noTab = substr( getAttribut( $tableauXml, 'ID' ), 1, strlen( getAttribut( $tableauXml, 'ID' ) ) - 1 );
			$link = '<a href="' . makeUrl( array( 'suiteId' => getAttribut( $tableauXml, 'DestinationDesElimines' ) ) ) . '" title="">Tableau des perdants</a>';
			
			if( $noTab < $maxTab )
				break;
				
			$label = '';
			if( $noTab == 0 )
				$label = 'Vainqueur';
			else if( $noTab == 1 )
				$label = 'Finale';
			else if( $noTab == 2 )
				$label = 'Demi-finale';
			else
				$label = 'Tableau de ' . $noTab;
			
			$head .= '
			<th>' . $label . '<br/>' . $link . '</th>';
		}
		
	}
	
	return $head;
}

function renderTHeaderOld( $startPowCountRequest )
{
	$head = '';
	
	$currentPow = $startPowCountRequest;
	$maxTab = 1;
	if( isset( $_POST[ 'tabEnd' ] ) )
		$maxTab = $_POST[ 'tabEnd' ];
	while( pow( 2, $currentPow ) >= $maxTab )
	{
		$label = '';
		if( $currentPow == 0 )
			$label = 'Vainqueur';
		else if( $currentPow == 1 )
			$label = 'Finale';
		else if( $currentPow == 2 )
			$label = 'Demi-finale';
		else
			$label = 'Tableau de ' . pow( 2, $currentPow );
		
		$head .= '
		<th>' . $label . '</th>';
		
		$currentPow--;
	}
	
	return $head;
}

function getTireurTab( $domXml, $phaseXml, $suiteXml, $startPowCount, $minInd, $maxInd )
{
	$t128 = array ( '1','','128','','65','','64','','33','','96','','97','','32','','17','','112','','81','','48','','49','','80','','113','','16','','9','','120','','73','','56','','41','','88','','105','','24','','25','','104','','89','','40','','57','','72','','121','','8','','5','','124','','69','','60','','37','','92','','101','','28','','21','','108','','85','','44','','53','','76','','117','','12','','13','','116','','77','','52','','45','','84','','109','','20','','29','','100','','93','','36','','61','','68','','125','','4','','3','','126','','67','','62','','35','','94','','99','','30','','19','','110','','83','','46','','51','','78','','115','','14','','11','','118','','75','','54','','43','','86','','107','','22','','27','','102','','91','','38','','59','','70','','123','','6','','7','','122','','71','','58','','39','','90','','103','','26','','23','','106','','87','','42','','55','','74','','119','','10','','15','','114','','79','','50','','47','','82','','111','','18','','31','','98','','95','','34','','63','','66','','127','','2');
	
	$tireurTab = array();
	$tempTireurRg = array();
	$tempWin = array();
	
	$searchLabel = ( $domXml->documentElement->localName == 'CompetitionParEquipes' ) ? 'Equipe' : 'Tireur';
	$tireurXml	= $phaseXml->getElementsByTagName( $searchLabel );
	
	//*** premier remplissage
	$currentPow = $startPowCount;
	$tCurrent = array();
	$currentTab = 0;
	$currentInd = 0;
	$i = 1;
	for( $ind = 0; $ind < count( $t128 ); $ind++ )
	{
		$current = $t128[ $ind ];
		
		if( ( $current <= $maxInd && $current >= $minInd ) )
		{
			$tCurrent[] = $current;
		}
	}
	
	foreach( $phaseXml->childNodes as $tireurXml )
	{
		if( $tireurXml->localName == $searchLabel )
		{
			$tireurInitRank = getTireurFirstRankInSuite( $phaseXml, getAttribut( $suiteXml, 'ID' ), getAttribut( $tireurXml, 'REF' ) );
			if( $tireurInitRank != -1 )
			{
				$tireurTab[ $currentPow ][ $tireurInitRank ] = array();
				$tireurTab[ $currentPow ][ $tireurInitRank ][ 'refTireur' ] = getAttribut( $tireurXml, 'REF' );
				
				$tempTireurRg[ getAttribut( $tireurXml, 'REF' ) ] = $tireurInitRank;
			}
		}
	}
	
	foreach( $suiteXml->childNodes as $tableauXml )
	{
		if( $tableauXml->localName == 'Tableau' )
		{
			$tabNum = substr( getAttribut( $tableauXml, 'ID' ), 1, strlen( getAttribut( $tableauXml, 'ID' ) ) - 1 );
			$currentPow = ( log( $tabNum ) ) / ( log( 2 ) );
			
			foreach( $tableauXml->childNodes as $matchXml )
			{
				if( $matchXml->localName == 'Match' )
				{
					foreach( $matchXml->childNodes as $tireurXml )
					{
						if( $tireurXml->localName == $searchLabel )
						{
							if( isset( $tempTireurRg[ getAttribut( $tireurXml, 'REF' ) ] ) )
							{
								if( !isset( $tireurTab[ $currentPow ][ $tempTireurRg[ getAttribut( $tireurXml, 'REF' ) ] ] ) )
								{
									$tireurTab[ $currentPow ][ $tempTireurRg[ getAttribut( $tireurXml, 'REF' ) ] ] = array();
									$tireurTab[ $currentPow ][ $tempTireurRg[ getAttribut( $tireurXml, 'REF' ) ] ][ 'refTireur' ] = getAttribut( $tireurXml, 'REF' );
								}
								
								$tireurTab[ $currentPow ][ $tempTireurRg[ getAttribut( $tireurXml, 'REF' ) ] ][ 'score'  ] = getAttribut( $tireurXml, 'Score' );
								$tireurTab[ $currentPow ][ $tempTireurRg[ getAttribut( $tireurXml, 'REF' ) ] ][ 'statut' ] = getAttribut( $tireurXml, 'Statut' );
								
								if( getAttribut( $tireurXml, 'Statut' ) == POULE_STATUT_VICTOIRE )
								{
									$tireurRg = min( $tempTireurRg[ getAttribut( $tireurXml, 'REF' ) ], getAdversaryRank( $tabNum, $tempTireurRg[ getAttribut( $tireurXml, 'REF' ) ], $minInd ) );
									
									if( !isset( $tireurTab[ $currentPow - 1 ][ $tireurRg ] ) )
									{
										$tireurTab[ $currentPow - 1 ][ $tireurRg ] = array();
										$tireurTab[ $currentPow - 1 ][ $tireurRg ][ 'refTireur' ] = getAttribut( $tireurXml, 'REF' );
										
										$tempTireurRg[ getAttribut( $tireurXml, 'REF' ) ] = $tireurRg;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	$tInd = 0;
	
	return $tireurTab;
}

/* 
* @param tab : numéro du tableau (16, 8...)
* @param rank : classement du tireur
* @param minInd : classement minimum de ce tableau (en cas de tableau des perdants)
*/
function getAdversaryRank( $tab, $rank, $minInd )
{
	return $tab - 1 - $rank + ( $minInd * 2 );
}


function getTireurFirstRankInSuite( $phaseXml, $suiteId, $tireurRef )
{
	$iniRg = -1;
	
	foreach( $phaseXml->childNodes as $tireurXml )
	{
		if( ( $tireurXml->localName == 'Tireur' || $tireurXml->localName == 'Equipe' ) && getAttribut( $tireurXml, 'REF' ) == $tireurRef )
			$iniRg = getAttribut( $tireurXml, 'RangInitial' );
	}
	
	// echo 'end of poule : ' . $iniRg . ' : <br/>';
	
	$suiteXml = getSuiteById( $phaseXml, '0' );
	if( getAttribut( $suiteXml, 'ID' ) === $suiteId )
		return $iniRg;
	
	$rank = $iniRg;
	$notDone = true;
	$cpt = 0;
	while( $notDone )
	{
		$minRank = substr( getAttribut( $suiteXml, 'Titre' ), 8, strlen( getAttribut( $suiteXml, 'Titre' ) ) - 1 );
	
		// echo getAttribut( $suiteXml, 'ID' ) . ' : <br/>';
		$rank = getTireurFinalRankInSuite( $suiteXml, $tireurRef, $rank, $minRank, $loosersSuite );
		
		// echo ' final rank : ' . $rank . '; lost to ' . $loosersSuite . '<br/>';
		if( $rank == $minRank )
			$notDone = false;
		else
		{
			$suiteXml = getSuiteById( $phaseXml, $loosersSuite );
			
			if( $suiteXml == null || getAttribut( $suiteXml, 'ID' ) === $suiteId )
				return $rank;
		}
		
		if( $cpt++ == 30 )
			$notDone = false;
	}
	
	return -1;
}

function getTireurFinalRankInSuite( $suiteXml, $tireurRef, $initialRank, $minRank, &$loosersSuite )
{
	foreach( $suiteXml->childNodes as $tableauXml )
	{
		if( $tableauXml->localName == 'Tableau' )
		{
			$tabNum = substr( getAttribut( $tableauXml, 'ID' ), 1, strlen( getAttribut( $tableauXml, 'ID' ) ) - 1 );
			foreach( $tableauXml->childNodes as $matchXml )
			{
				if( $matchXml->localName == 'Match' )
				{
					foreach( $matchXml->childNodes as $tireurXml )
					{
						if( $tireurXml->localName == 'Tireur' || $tireurXml->localName == 'Equipe' )
						{
							if( getAttribut( $tireurXml, 'REF' ) == $tireurRef )
							{
								if( getAttribut( $tireurXml, 'Statut' ) == POULE_STATUT_DEFAITE )
								{
									// echo ' lost against ' . getAdversaryRank( $tabNum, $initialRank, $minRank ) . '<br/>';
									$loosersSuite = getAttribut( $tableauXml, 'DestinationDesElimines' );
									return max( $initialRank, getAdversaryRank( $tabNum, $initialRank, $minRank ) );
								}
								else
								{
									// echo ' won against ' . getAdversaryRank( $tabNum, $initialRank, $minRank ) . '<br/>';
									$initialRank = min( $initialRank, getAdversaryRank( $tabNum, $initialRank, $minRank ) );
									break;
								}
							}
						}
					}
				}
			}
		}
	}
	
	return $initialRank;
}

function getSuiteById( $phaseXml, $suiteId )
{
	$suiteListXml = $phaseXml->getElementsByTagName( 'SuiteDeTableaux' );
	
	foreach( $suiteListXml as $suiteXml )
	{
		if( strcmp( getAttribut( $suiteXml, 'ID' ), $suiteId ) == 0 )
			return $suiteXml;
	}
	
	return null;
}

/********************************************************/

/*                   CLASSEMENT GENERAL                 */

/********************************************************/
function renderClassement( $domXml )
{
	$list = '';
	
	$tireurCount = 0;
	
	$searchLabelParent = ( $domXml->documentElement->localName == 'CompetitionParEquipes' ) ? 'Equipes' : 'Tireurs';
	$searchLabelChildren = ( $domXml->documentElement->localName == 'CompetitionParEquipes' ) ? 'Equipe' : 'Tireur';
	
	$tireursXml	= $domXml->getElementsByTagName( $searchLabelParent );
	foreach ($tireursXml as $tireurs) 
	{
		$tireurXml = $tireurs->getElementsByTagName( $searchLabelChildren );
		$tireurCount = 0;
		 
		foreach ($tireurXml as $tireur) 
		{
			if( getAttribut( $tireur, 'Statut' ) != STATUT_ABSENT )
				$tireurCount++;
		}
	}
	
	$list .= '
	<table class="listeTireur">
		<tr>
			<th>Rang</th>
			<th>Nom</th>';
			
			if( $searchLabelChildren == 'Tireur' )
			{
				$list .= '
				<th>Prénom</th>';
			}
			
			$list .= '
			<th>Club</th>
		</tr>';
		
		$i = 1;
		$pair = "pair";
		while( $i <= $tireurCount )
		{
			foreach ($tireursXml as $tireurs) 
			{
				$tireurXml = $tireurs->getElementsByTagName( $searchLabelChildren );
				
				foreach ($tireurXml as $tireur) 
				{
					if( getAttribut( $tireur, 'Classement' ) == $i )
					{
						$list .= '
						<tr class="'. $pair . '">
							<td>' . getAttribut( $tireur, 'Classement' ) . '</td>
							<td>' . getAttribut( $tireur, 'Nom' ) . '</td>';
							
							if( $searchLabelChildren == 'Tireur' )
							{
								$list .= '
								<td>' . getAttribut( $tireur, 'Prenom' ) . '</td>';
							}
							
							$list .= '
							<td>' . getAttribut( $tireur, 'Club' ) . '</td>
						</tr>';
						
						$pair = $pair == "pair" ? "impair" : "pair";
					}
				}
			}
			
			$i++;
		}
		
	$list .= '
	</table>';
	
	return $list;

}
?>