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
echo "nc_inq()=".nc_inq($q,&$ndimsp,&$nvarsp,&$ngattsp,&$unlimdimidp);
echo "ndims=".$ndimsp.",nvars=".$nvarsp.",ngatts=".$ngattsp.",unlimdimid=".$unlimdimidp;
echo "nc_close()=".nc_close($q)."\n";
?>
