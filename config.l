/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <joris.dedieu@gmail.com> wrote this file. As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return. Joris Dedieu
 * ----------------------------------------------------------------------------
 *
 *
 */

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
^daemon				{ return DAEMON; }
[0-9]+				{ yylval.val = atoi(yytext); return INTEGER; }
\/[a-zA-z\/0-9._-]+		{ yylval.string = yytext; return PATH; }
\"[^\"]*\"			{ yylval.string = yytext; return STRING; }
<<EOF>>				{ return 0; }
%%





