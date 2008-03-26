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
$filename="/home/andrew/brandies.nc";
echo "nc_open($filename,0)=".nc_open($filename,0,&$q)."\n";
echo "ncid=".$q."\n";
echo "nc_inq()=".nc_inq($q,&$ndims,&$nvars,&$ngatts,&$unlimdimid)."\n";
echo "ndims=".$ndims.",nvars=".$nvars.",ngatts=".$ngatts.",unlimdimid=".$unlimdimid,"\n";
echo "Dimensions:\n";
for ($i=0; $i<$ndims; $i++)
{
  nc_inq_dim($q, $i, &$name, &$length);
  echo "\t".$name."\n";
}
echo "Variables:\n";
for ($i=0; $i<$nvars; $i++)
{
  nc_inq_varname($q, $i, &$name);
  echo "\t".$name."\n";
}
echo "Global attributes:\n";
for ($i=0; $i<$ngatts; $i++)
{
  nc_inq_attname($q, -1, $i, &$name);
  echo "\t".$name."\n";
}
echo "nc_close()=".nc_close($q)."\n";
?>
