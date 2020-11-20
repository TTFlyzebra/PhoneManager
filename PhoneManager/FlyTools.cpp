#include "stdafx.h"
#include "FlyTools.h"


 void TRACE(char *format, ...) { 
	char out[1024]; 
	va_list list; 
	va_start(list, format);	
	wvsprintf(out, format, list); 
	va_end(list);	
	OutputDebugString(out); 	
 } 