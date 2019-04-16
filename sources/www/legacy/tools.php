<?php

define( "STATUT_PRESENT", "Q" );
define( "STATUT_ABSENT", "F" );
define( "STATUT_ELIMINE", "N" );

define( "POULE_STATUT_VICTOIRE", "V" );
define( "POULE_STATUT_DEFAITE", "D" );
define( "POULE_STATUT_ABANDON", "A" );
define( "POULE_STATUT_EXPULSION", "E" );

define( "SEXE_MALE", "M" );
define( "SEXE_FEMALE", "F" );

function endsWith( $str, $sub ) 
{
  return ( substr( $str, strlen( $str ) - strlen( $sub ) ) == $sub );
}

function explorer($chemin)
{
  $lstat    = lstat($chemin);
  $mtime    = date('d/m/Y H:i:s', $lstat['mtime']);
  $filetype = filetype($chemin);

  $competitionList = Array();

  if( endswith( $chemin, ".cotcot" ) )
    $competitionList[] = $chemin;

  // Affichage des infos sur le fichier $chemin
  //echo "$chemin   type: $filetype size: $lstat[size]  mtime: $mtime\n";

  // Si $chemin est un dossier => on appelle la fonction explorer() pour chaque élément (fichier ou dossier) du dossier$chemin
  if( is_dir($chemin) )
  {
    $me = opendir($chemin);
    while( $child = readdir($me) )
    {
      if( $child != '.' && $child != '..' )
      {
        $competitionList = array_merge( $competitionList, explorer( $chemin.DIRECTORY_SEPARATOR.$child ) );
      }
    }
  }

  return $competitionList;
}

function selectedCompetition()
{
  if( isset( $_GET[ 'idCompetition' ] ) )
    return $_GET[ 'idCompetition' ];

  return -1;
}

function selectedPhase()
{
  if( isset( $_GET[ 'phaseId' ] ) )
    return $_GET[ 'phaseId' ];

  return -1;
}

function selectedPhaseName( $domXml )
{
  if( isset( $_GET[ 'phaseId' ] ) )
  {
    $phase = getPhaseXmlElement( $domXml, $_GET[ 'phaseId' ] );

    if( $phase != null )
      return $phase->tagName;
  }

  return 'ListeInscrit';
}

function getPhaseXmlElement( $docXml, $idPhase )
{
  $phasesXml = $docXml->getElementsByTagName( 'Phases' );

  foreach( $phasesXml as $phases ) 
  {
    foreach($phases->childNodes as $phase)
    {
      if( isset( $phase->tagName ) && getAttribut( $phase, 'PhaseID' ) == $idPhase )
        return $phase;
    }
  }

  return null;
}

function makeUrl($paramsOverlay, $clear=0) 
{
  $url = 'index.php?action=affichage';

  if ($clear == 0)
  {
    if( isset( $_GET[ 'refresh' ] ) )
      unset( $_GET[ 'refresh' ] );

    $params = array_merge($_GET, $paramsOverlay);
  }
  else 
    $params = $paramsOverlay;

  foreach ($params as $k=>$v) if (! is_null($v))
  {
    $url.= '&'.$k.'='.$v;
  }
  return $url;
}

/*************
     XML
*************/
function getAttribut( $xmlElement, $attributName )
{
  return $xmlElement->getAttribute( $attributName );
}

function fillSessionWithTitreLong()
{
  if( isset( $_SESSION[ 'cotcotFiles' ] ) )
  {
    $DOMxml = new DOMDocument( '1.0', 'utf-8' );
    $titre = array();

    foreach( $_SESSION[ 'cotcotFiles' ] as $file )
    {
      $DOMxml->load( $file );

      $competXml = $DOMxml->documentElement;
      $titre[] = getAttribut( $competXml, 'TitreLong' );
    }

    $_SESSION[ 'titreList'] = $titre;
  }
}

function getCategorieLibelle( $categorie )
{
  switch( $categorie )
  {
  case 'B':
    return 'Benjamin';
  case 'M':
    return 'Minime';
  case 'C':
    return 'Cadet';
  case 'J':
    return 'Junior';
  case 'S':
    return 'Sénior';
  case 'V':
    return 'Vétéran';
  }
}

function getSexeLabel( $sexe )
{
  switch( $sexe )
  {
  case SEXE_MALE:
    return 'Homme';
  case SEXE_FEMALE:
    return 'Dame';
  }
}
?>
