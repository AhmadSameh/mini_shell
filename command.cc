
/*
 * CS354: Shell project
 *
 * Template file.
 * You will need to add more code here to execute the command table.
 *
 * NOTE: You are responsible for fixing any bugs this code may have!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>

#include <dirent.h>
#include <limits.h>
#include <time.h>

#include "command.h"

SimpleCommand::SimpleCommand() {
	// Creat available space for 5 arguments
	_numberOfAvailableArguments = 5;
	_numberOfArguments = 0;
	_arguments = (char **) malloc( _numberOfAvailableArguments * sizeof( char * ) );
}

void SimpleCommand::insertArgument( char * argument ) {
	if ( _numberOfAvailableArguments == _numberOfArguments  + 1 ) {
		// Double the available space
		_numberOfAvailableArguments *= 2;
		_arguments = (char **) realloc( _arguments,
				  _numberOfAvailableArguments * sizeof( char * ) );
	}
	
	_arguments[ _numberOfArguments ] = argument;

	// Add NULL argument at the end
	_arguments[ _numberOfArguments + 1] = NULL;
	
	_numberOfArguments++;
}

Command::Command(){
	// Create available space for one simple command
	_numberOfAvailableSimpleCommands = 1;
	_simpleCommands = (SimpleCommand **)
		malloc( _numberOfSimpleCommands * sizeof( SimpleCommand * ) );

	chdir(getenv("HOME"));
	getcwd(_currentDir, sizeof(_currentDir));

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
}

void Command::insertSimpleCommand( SimpleCommand * simpleCommand ){
	if ( _numberOfAvailableSimpleCommands == _numberOfSimpleCommands ) {
		_numberOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **) realloc( _simpleCommands,
			 _numberOfAvailableSimpleCommands * sizeof( SimpleCommand * ) );
	}
	
	_simpleCommands[ _numberOfSimpleCommands ] = simpleCommand;
	_numberOfSimpleCommands++;
}

void Command:: clear(){
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		for ( int j = 0; j < _simpleCommands[ i ]->_numberOfArguments; j ++ ) {
			free ( _simpleCommands[ i ]->_arguments[ j ] );
		}
		
		free ( _simpleCommands[ i ]->_arguments );
		free ( _simpleCommands[ i ] );
	}

	/*if ( _outFile ) {
		free( _outFile );
	}

	if ( _inputFile ) {
		free( _inputFile );
	}

	if ( _errFile ) {
		free( _errFile );
	}*/

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
}

void Command::print() {
	printf("\n\n");
	printf("              COMMAND TABLE                \n");
	printf("\n");
	printf("  #   Simple Commands\n");
	printf("  --- ----------------------------------------------------------\n");
	
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		printf("  %-3d ", i );
		for ( int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++ ) {
			printf("\"%s\" \t", _simpleCommands[i]->_arguments[ j ] );
		}
	}

	printf( "\n\n" );
	printf( "  Output       Input        Error        Background\n" );
	printf( "  ------------ ------------ ------------ ------------\n" );
	printf( "  %-12s %-12s %-12s %-12s\n", _outFile?_outFile:"default",
		_inputFile?_inputFile:"default", _errFile?_errFile:"default",
		_background?"YES":"NO");
	printf( "\n\n" );
	
}

void Command::execute() {

	// Don't do anything if there are no simple commands
	if ( _numberOfSimpleCommands == 0 ) {
		prompt();
		return;
	}

	// Print contents of Command data structure
	print();


	int defaultin = dup( 0 ); // Default file Descriptor for stdin
	int defaultout = dup( 1 ); // Default file Descriptor for stdout
	int defaulterr = dup( 2 ); // Default file Descriptor for stderr

	
	int outfd;
	if(_appendFlag)
		outfd = open(_outFile, O_APPEND|O_RDWR,0666);
	else
		outfd = creat(_outFile, 0666);
	int errfd;
	if(_appendFlag)
		errfd = open(_outFile, O_APPEND|O_RDWR,0666);
	else
		errfd = creat(_outFile, 0666);
	int infd  = open(_inputFile, 0666);
	_appendFlag = false;

	char* to;
	char* dir;
	char* gdir;
	char buf[1000];


	int pipes = _numberOfSimpleCommands - 1;
	int fdpipe[pipes][2];
	for(int i=0; i<pipes; i++){
		if ( pipe(fdpipe[i]) == -1) {
			perror( "Pipe error");
			exit( 2 );
		}
	}

	if (!strcmp(_simpleCommands[0]->_arguments[0], "cd")){

			if(!strcmp(_simpleCommands[0]->_arguments[1], "..")){
				chdir("..");
				gdir = getcwd(buf, sizeof(buf));
				strcpy(to, gdir);
			}else{
            	gdir = getcwd(buf, sizeof(buf));
            	dir = strcat(gdir, "/");
            	to = strcat(dir, _simpleCommands[0]->_arguments[1]);
				chdir(to);
			}

			strcpy(_currentDir, to);

    }else{
		int pid;
		for(int i=0; i<_numberOfSimpleCommands; i++){

			if(_numberOfSimpleCommands > 1){
				if(i == 0){
					// Redirect input (use sdtin)
					dup2(defaultin, 0 );

					// Redirect output to pipe (write the output to pipefile[1] instead od stdout)
					dup2(fdpipe[i][1], 1 );
					close(fdpipe[i][1]);

					// Redirect err (use stderr)
					dup2( defaulterr, 2 );
				}else if( i < _numberOfSimpleCommands - 1 && i > 0){
					dup2(fdpipe[i - 1][0], 0);
					close(fdpipe[i - 1][0]);
					// Redirect output to pipe (write the output to pipefile[1] instead od stdout)
					dup2( fdpipe[i][1], 1 );
					close(fdpipe[i][1]);
	
					// Redirect err
					dup2(defaulterr, 2);
				}else if(i == _numberOfSimpleCommands - 1){
					dup2( fdpipe[i - 1][0], 0);
					close(fdpipe[i - 1][0]);
					// Redirect Output to the Default (stdout)
					dup2( defaultout ,1);
	
					// Redirect err
					dup2(defaulterr, 2);
				}
			}
			if(_outFile != 0){
				// Redirect output to the created utfile instead off printing to stdout 
				if(_numberOfSimpleCommands > 1 && i == _numberOfSimpleCommands - 1){
					dup2(outfd, 1);
					close(outfd);
				}else if(_numberOfSimpleCommands == 1){
					dup2(outfd, 1);
					close(outfd);
				}
			}
			if(_inputFile != 0){
				// Redirect input
				if(_numberOfSimpleCommands > 1 && i == 0){
					dup2(infd, 0);
					close(infd);
				}else if(_numberOfSimpleCommands == 1){
					dup2(infd, 0);
					close(infd);
				}
			}
			if(_errFile != 0){
				if(_numberOfSimpleCommands > 1 && i == _numberOfSimpleCommands - 1){
					dup2(errfd, 2);
					close(errfd);
				}else if(_numberOfSimpleCommands == 1){
					dup2(errfd, 2);
					close(errfd);
				}
			}

			// Create new process for "cat"
			pid = fork();
			if ( pid == -1 ) {
				perror( "Fork error\n");
				exit( 2 );
			}

			if (pid == 0) {
				//Child
		
				// close file descriptors that are not needed
				//close(fdpipe[i][0]);
				//close(fdpipe[i][1]);
				close( defaultin );
				close( defaultout );
				close( defaulterr );

				if(!_background)
					waitpid(pid, 0, 0);

				// You can use execvp() instead if the arguments are stored in an array
				execvp(_simpleCommands[i]->_arguments[0], _simpleCommands[i]->_arguments);

				perror("incorrect command");
				exit( 2 );
			}
		}
		// Restore input, output, and error

		dup2( defaultin, 0 );
		dup2( defaultout, 1 );
		dup2( defaulterr, 2 );

		// Close file descriptors that are not needed
		//close(fdpipe[0]);
		//close(fdpipe[1]);
		close( defaultin );
		close( defaultout );
		close( defaulterr );

		//_background = false;
		// Wait for last process in the pipe line
		if(!_background)
			waitpid( pid, 0, 0 );
	}

	clear();
	prompt();
}

// Shell implementation

void Command::prompt(){
	printf("BOI_shell:%s$ ", _currentDir);
	fflush(stdout);
}

Command Command::_currentCommand;
SimpleCommand * Command::_currentSimpleCommand;

int yyparse(void);

void c_handler(int dummy){
	//clear();
	//printf("\n");
	//prompt();
	return;
}

void add_to_log(int dummy){
	time_t rawtime;
 	struct tm * timeinfo;
  	time ( &rawtime );
  	timeinfo = localtime ( &rawtime );

	FILE* log;
	log = fopen("log.txt", "a");
	fprintf(log, "Process Terminated @ %s", asctime (timeinfo));

	fclose(log);
}

int main(){
	signal(SIGINT, c_handler);
	signal(SIGCHLD, add_to_log);
	Command::_currentCommand.prompt();
	yyparse();
	return 0;
}

