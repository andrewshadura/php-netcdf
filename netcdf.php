<?php
$br = (php_sapi_name() == "cli")? "":"<br>";

if(!extension_loaded('netcdf')) {
	dl('netcdf.' . PHP_SHLIB_SUFFIX);
}
$module = 'netcdf';
$functions = get_extension_funcs($module);
echo "Functions available in the test extension:$br\n";
foreach($functions as $func) {
    echo $func."$br\n";
}
echo "$br\n";
$filename=$argv[1];
echo "nc_open($filename,0)=".nc_open($filename,0,$q)."\n";
echo "ncid=".$q."\n";
echo "nc_inq()=".nc_inq($q,$ndims,$nvars,$ngatts,$unlimdimid)."\n";
echo "ndims=".$ndims.",nvars=".$nvars.",ngatts=".$ngatts.",unlimdimid=".$unlimdimid,"\n";
echo "Dimensions:\n";
for ($i=0; $i<$ndims; $i++)
{
  nc_inq_dim($q, $i, $name, $length);
  echo "\t$name     $i       $length\n";
  $dim_names[$i] = $name;
}
echo "Variables:\n";
for ($i=0; $i<$nvars; $i++)
{
  /*nc_inq_varname($q, $i, &$name);
  nc_inq_vartype($q, $i, &$type);
  nc_inq_varndims($q, $i, &$dims);
  nc_inq_vardimid($q, $i, &$dimids);
  nc_inq_varnatts($q, $i, &$natts); */
  nc_inq_var($q, $i, $name, $type, $dims, $dimids, $natts);
  $var_names[$i] = $name;
  $var_num_dims[$i] = $dims;

  $var_dims = "(";
  foreach($dimids as $d)
    $var_dims .= " {$dim_names[$d]} ";
  $var_dims .= ")";
  
  $var_natts = "";
  for($j=0; $j<$natts; $j ++){
    nc_inq_attname($q, $i, $j, $n);
	nc_get_att($q, $i, $n, $val);
	if(is_array($val)){
      $var_natts .= "            $n : (".implode(' , ',$val)." )\n";
	}
	else 
	  $var_natts .= "            $n : $val\n";
  }
  $var_natts .= "\n";

  echo "\t$name       ".nc_strtype($type)."     $dims  $var_dims      $natts\n$var_natts\n";
}
echo "Global attributes:\n";
for ($i=0; $i<$ngatts; $i++)
{
  //nc_inq_attname($q, -1, $i, &$name);
  nc_inq_attname($q, NC_GLOBAL, $i, $name);
  nc_get_att($q, NC_GLOBAL, $name, $val);

  echo "\t$name = $val\n";
}

for ($i=0; $i<$nvars; $i++)
{
  if($var_num_dims[$i]==1){
    nc_get_var1($q, $i, array(0), $val);
    nc_get_vara($q, $i, array(0), array(1), $values);	
  }
  elseif($var_num_dims[$i]==2){
    nc_get_var1($q, $i, array(0,0), $val);
    nc_get_vara($q, $i, array(0,0), array(1,1), $values);	
  }
  if($var_num_dims[$i]==3){
    nc_get_var1($q, $i, array(0,0,0), $val);
    nc_get_vara($q, $i, array(0,0,0), array(1,1,1), $values);
  }
  echo "$i: {$var_names[$i]}(0,0) = $val\n";
  if(is_array($values))
    echo "values(0,0) = ".implode(' , ',$values)."\n\n";
  else echo "values(0,0) = $values\n\n";
}

/*
nc_get_var($q,26, &$values);
echo "\n{$var_names[26]}:\n";
if(is_array($values))
  echo "values = ".implode(' , ',$values)."\n\n";
else echo "values = $values\n\n";

for($i=1;$i<7;$i++){
nc_get_vars($q, 26, array(0), array(min(4,1+(int)((11-1)/$i))), array($i), &$values);
echo "\n{$var_names[26]} (stride $i):\n";
if(is_array($values))
  echo "values = ".implode(' , ',$values)."\n\n";
else echo "values = $values\n\n";
}*/

echo "\nHEADER:\n";
nc_dump_header($q,$header);
print_r($header);

echo "\nVALUES OF {$var_names[0]},{$var_names[1]}:\n";
nc_get_values($q, $allvalues,array($var_names[0],$var_names[1]));
print_r($allvalues);

echo "\nVALUES OF FIRST 2 blocks:\n";
for($i=0;$i<2;$i++){
	nc_get_values($q, $allvalues,null, array($i), array(1),null);
	print_r($allvalues);
}
echo "nc_close()=".nc_close($q)."\n";
?>
