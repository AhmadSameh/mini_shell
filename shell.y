
/*
 * CS-413 Spring 98
 * shell.y: parser for shell
 *
 * This parser compiles the following grammar:
 *
 *	cmd [arg]* [> filename]
 *
 * you must extend it to understand the complete shell grammar
 *
 */

%start goal

%token	<string_val> WORD

%token 	NOTOKEN GREAT APPEND LESS BCKGND NEWLINE PIPE EXIT TEST ERR_OUT ERR_APP

%union	{
		char   *string_val;
	}

%{
extern "C" 
{
	int yylex();
	void yyerror (char const *s);
}
#define yylex yylex
#include <stdio.h>
#include "command.h"
%}

%%

goal:	
	commands
	;

commands: 
	command
	| commands command 
	;

command: 
		simple_command
    	;

simple_command:	
	EXIT NEWLINE{
			printf("\n\n GOODBYE!! \n\n");
			exit(1);
		}
	|
	command_and_args NEWLINE {
		printf("   Yacc: Execute command\n");
		Command::_currentCommand.execute();
	}
	|
	command_and_args iomodifier_opt NEWLINE {
		printf("   Yacc: Execute command\n");
		Command::_currentCommand.execute();
	}
	| NEWLINE 
	| error NEWLINE { yyerrok; }
	;

command_and_args:
	command_word arg_list {
		Command::_currentCommand.
		insertSimpleCommand(Command::_currentSimpleCommand);
	}
	;

arg_list:
	arg_list argument
	| /* can be empty */
	;

argument:
	WORD {
        printf("   Yacc: insert argument \"%s\"\n", $1);
	    Command::_currentSimpleCommand->insertArgument( $1 );\
	}
	;

command_word:
	WORD {
        printf("   Yacc: insert command \"%s\"\n", $1);   
	    Command::_currentSimpleCommand = new SimpleCommand();
	    Command::_currentSimpleCommand->insertArgument( $1 );
	}
	;

iomodifier_opt:
	GREAT WORD {
		printf("   Yacc: insert output \"%s\"\n", $2);
		Command::_currentCommand._outFile = $2;
	}
	|
	LESS WORD{
		printf("   Yacc: insert input \"%s\"\n", $2);
		Command::_currentCommand._inputFile = $2;
	}
	|
	LESS WORD GREAT WORD{
		printf("   Yacc: insert input \"%s\"\n", $2);
		Command::_currentCommand._inputFile = $2;
		printf("   Yacc: insert output \"%s\"\n", $4);
		Command::_currentCommand._outFile = $4;
	}
	|
	APPEND WORD{
		printf("   Yacc: append output \"%s\"\n", $2);
		Command::_currentCommand._outFile = $2;
		Command::_currentCommand._appendFlag = 1;
	}
	|
	ERR_OUT WORD{
		printf("   Yacc: insert output \"%s\"\n", $2);
		Command::_currentCommand._outFile = $2;
		printf("   Yacc: error output \"%s\"\n", $2);
		Command::_currentCommand._errFile = $2;
	}
	|
	ERR_APP WORD{
		printf("   Yacc: insert output \"%s\"\n", $2);
		Command::_currentCommand._outFile = $2;
		printf("   Yacc: error output \"%s\"\n", $2);
		Command::_currentCommand._errFile = $2;
		Command::_currentCommand._appendFlag = 1;
	}
	|
	BCKGND{
		Command::_currentCommand._background = 1;
	}
	|
	PIPE goal
	|
	iomodifier_opt PIPE goal
	;

%%

void yyerror(const char * s){
	fprintf(stderr,"%s", s);
}

#if 0
main(){
	yyparse();
}
#endif
