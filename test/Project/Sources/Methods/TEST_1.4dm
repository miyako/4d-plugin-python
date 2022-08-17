//%attributes = {}
/*
to evaluate a statement; the value type must be object
*/

$status:=python("{\"value\" : 5 + 5 }"; New object:C1471; python_eval)
//Py_eval_input (isolated expressions)

/*
this with evalute the script like a module but return nothing. this is the default
*/

$status:=python("from time import time,ctime\n\nprint('Today is', ctime(time()))\n"; New object:C1471; python_exec)
//Py_file_input (sequences of statements)

/*
not sure if this is useful, because it is intended for interactive loops but the plugin is one-shot
*/

$status:=python("{\"value\" : 5 + 5 }"; New object:C1471; python_line)
//Py_single_input (single statement)