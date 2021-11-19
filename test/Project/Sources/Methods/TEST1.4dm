//%attributes = {}
/*
import fourd
d = {"foo":"bar"};
fourd.call("cb_test", d);
*/

  //invoke the project method "cb_test" passing d as $1 (C_OBJECT)

METHOD GET CODE:C1190(Current method path:C1201;$code)

ARRAY LONGINT:C221($pos;0)
ARRAY LONGINT:C221($len;0)

If (Match regex:C1019("(?s)(?:\\/\\*)(.*)(?:\\*\\/)";$code;1;$pos;$len))
	$py:=Substring:C12($code;$pos{1};$len{1})
	$status:=python ($py)
End if 
