//%attributes = {}
/*
import fourd
fourd.call("cb_test", locals());
fourd.call("cb_test", locals());
*/

//invoke the project method "cb_test" passing locals() as $1 (C_OBJECT)

METHOD GET CODE:C1190(Current method path:C1201; $code)

$locals:=New object:C1471("foo"; "bar")

ARRAY LONGINT:C221($pos; 0)
ARRAY LONGINT:C221($len; 0)

If (Match regex:C1019("(?s)(?:\\/\\*)(.*)(?:\\*\\/)"; $code; 1; $pos; $len))
	$py:=Substring:C12($code; $pos{1}; $len{1})
	$status:=python($py; $locals)  //warning! do not break the callback method!
End if 
