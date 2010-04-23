%{
#include "config_yy.h"
%}


separator	[\t ]+

%%
^[#;].*$			;
^{separator}*			;
{separator}			;
\n				{ return EOL; }
^socket 			{ return SOCKPATH; }
^size 				{ return SIZE; }
^user				{ return USER; }
^pass				{ return PASS; }
[0-9]+				{ yylval.val = atoi(yytext); return INTEGER; }
\/[a-zA-z\/0-9._-]+		{ yylval.string = yytext; return PATH; }
\"[^\"]*\"			{ yylval.string = yytext; return STRING; }
<<EOF>>				{ return 0; }
%%





