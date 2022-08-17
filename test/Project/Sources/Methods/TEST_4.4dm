//%attributes = {}
$py:=Folder:C1567(fk resources folder:K87:11).file("join.py").getText()

$locals:=New object:C1471("foo"; "bar")

$status:=python($py; $locals; python_exec)

$returnValue:=PYTHON_RESULT