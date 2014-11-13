<?php

$folder = '';
$competString = '';

if( isset( $_POST[ 'list' ] ) )
{
	$folder = $_POST[ 'folder' ];
	$competList = explorer( $folder );
	foreach( $competList as $compet )
	{
		$competString .= $compet . '<br/>';
	}
}

echo '
<div class="selectFolder">
	<form action="" method="post" name="selectFolder">
		<p>Entrez le chemin du dossier contenant les fichiers compétitions (.cotcot). Les sous dossiers seront contrôlés</p>
		<div class="selectFolder_Button">
			<input type="text" name="folder" size="50" value="' . $folder . '" />
			<input type="submit" value="Contrôler" name="list"/>
			<input type="submit" value="Valider" name="validFolder"/>
		</div>
	</form>';
	
	if( $competString != '' )
	{
		echo '
		<p>Fichiers cotcot trouvés :<br/>' .
		$competString . '
		</p>';
	}

echo '
</div>';
?>
