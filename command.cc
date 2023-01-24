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
#include <sys/resource.h>
#include <string.h>
#include <signal.h>
#include <wait.h>
#include <fcntl.h>
#include <signal.h>
#include <fstream>
#include "command.h"

int parent_id;

void handler(int sig){
	pid_t pid;
	int stat;
	pid = wait(&stat);
	
	std::ofstream outfile;
  	outfile.open("logs.txt", std::ios_base::app);
  	outfile << "child process with parent id=" << parent_id << " terminated\n"; 

	return;
}

void signal_handler(int sig){
	signal(SIGINT, signal_handler);
	printf("\n");
	Command::_currentCommand.prompt();
	fflush(stdout);
	return;
}

SimpleCommand::SimpleCommand()
{
	// Creat available space for 5 arguments
	_numberOfAvailableArguments = 5;
	_numberOfArguments = 0;
	_arguments = (char **) malloc( _numberOfAvailableArguments * sizeof( char * ) );
}

void
SimpleCommand::insertArgument( char * argument )
{
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

Command::Command()
{
	// Create available space for one simple command
	_numberOfAvailableSimpleCommands = 1;
	_simpleCommands = (SimpleCommand **)
		malloc( _numberOfSimpleCommands * sizeof( SimpleCommand * ) );

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_catFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
}

void
Command::insertSimpleCommand( SimpleCommand * simpleCommand )
{
	if ( _numberOfAvailableSimpleCommands == _numberOfSimpleCommands ) {
		_numberOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **) realloc( _simpleCommands,
			 _numberOfAvailableSimpleCommands * sizeof( SimpleCommand * ) );
	}
	
	_simpleCommands[ _numberOfSimpleCommands ] = simpleCommand;
	_numberOfSimpleCommands++;
}

void
Command:: clear()
{
	for ( int i = 0; i < _numberOfSimpleCommands; i++ ) {
		for ( int j = 0; j < _simpleCommands[ i ]->_numberOfArguments; j ++ ) {
			free ( _simpleCommands[ i ]->_arguments[ j ] );
		}
		
		free ( _simpleCommands[ i ]->_arguments );
		free ( _simpleCommands[ i ] );
	}

	if ( _outFile && _outFile != _errFile ) {
		free( _outFile );
	}
	
	if ( _catFile ) {
		free( _catFile );
	}

	if ( _inputFile ) {
		free( _inputFile );
	}

	if ( _errFile ) {
		free( _errFile );
	}

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_catFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
}

void
Command::print()
{
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
	
	if (_outFile){
	printf( "  %-12s %-12s %-12s %-12s\n", _outFile?_outFile:"default",
		_inputFile?_inputFile:"default", _errFile?_errFile:"default",
		_background?"YES":"NO");
		}
	else{
	printf( "  %-12s %-12s %-12s %-12s\n", _catFile?_catFile:"default",
		_inputFile?_inputFile:"default", _errFile?_errFile:"default",
		_background?"YES":"NO");
	printf( "\n\n" );
	}
	
	printf( "\n\n" );
	
}

void 
Command:: excecute_command(int i)
{

	int pid = fork();
		if ( pid == -1 ) {
			perror( "ls: fork\n");
			exit( 2 );
		}

		if (pid == 0) {
			execvp(_simpleCommands[i]->_arguments[0], _simpleCommands[i]->_arguments);
			// exec() is not suppose to return, something went wrong
			// perror( "ls: exec ls");
			exit( 2 );
		}
		
		// Wait for last process in the pipe line
		waitpid( pid, 0, 0 );
	}

void
Command::execute()
{
	// Don't do anything if there are no simple commands
	if ( _numberOfSimpleCommands == 0 ) {
		prompt();
		return;
	}

	// Print contents of Command data structure
	print();

	/* if more than 1 */
	int defaultin = dup( 0 );
	int defaultout = dup( 1 );
	int defaulterr = dup( 2 );		
	
	if (_numberOfSimpleCommands > 1){
	int pid0;
	int fdpipe[_numberOfSimpleCommands-1][2];

	for (int i=0; i < _numberOfSimpleCommands-1; i++){
		if (pipe(fdpipe[i]) == -1)
			perror("Pipes not made!");
	}
	int infd = defaultin;
	int outfd = defaultout;
	for(int i=0;i<_numberOfSimpleCommands;i++){
		parent_id = getpid();
		signal(SIGCHLD, handler);
		if (strcmp(_simpleCommands[i]->_arguments[0], "exit") == 0)  
		{
    		printf("Goodbye...\n");
    		exit(0);
		}
		
		if ( _inputFile ) {
			infd = open(_inputFile, O_RDONLY);
			}
		if (_outFile){
			outfd = open(_outFile, O_CREAT | O_TRUNC | O_WRONLY, 0777);
		}
		if (_catFile){
			outfd = open(_catFile, O_CREAT | O_WRONLY | O_APPEND, 0777);
			}
		if(i==0){
			dup2( infd, 0 );
			dup2( fdpipe[0][1], 1 );
		}
		else if(i==_numberOfSimpleCommands-1){
			dup2( fdpipe[_numberOfSimpleCommands-2][0], 0 );
			dup2( outfd, 1 );	
		}
		else{
			dup2( fdpipe[i-1][0], 0 );
			dup2( fdpipe[i][1], 1 );
		}
	
		dup2( defaulterr, 2 );
		
		pid0 = fork();
		if ( pid0 == -1 ) {
			perror( "cat_grep: fork\n");
			exit( 0 );
		}

		if (pid0 == 0) {
		for (int j=0;j<_numberOfSimpleCommands-1;j++){
			close(fdpipe[j][0]);
			close(fdpipe[j][1]);
			}
			close( infd );
			close( outfd );
			close( defaulterr );
		execvp(_simpleCommands[i]->_arguments[0], _simpleCommands[i]->_arguments);
		// exec() is not suppose to return, something went wrong
		perror( "cat_grep: exec cat");
		exit( 0 );
		}
	}	
        dup2( defaultin, 0 );
	dup2( defaultout, 1 );
	dup2( defaulterr, 2 );
	for (int j=0;j<_numberOfSimpleCommands-1;j++){
		close(fdpipe[j][0]);
		close(fdpipe[j][1]);
	}
	close( defaultin );
	close( defaultout );
	close( defaulterr );
	waitpid( pid0, 0, 0 );
	}

	else if ( _numberOfSimpleCommands == 1 ){
	if (strcmp(_simpleCommands[0]->_arguments[0], "exit") == 0) {
    		printf("Goodbye...\n");
    		exit(0);
	}
	if (strcmp(_simpleCommands[0]->_arguments[0], "cd") == 0) {
    		char s[100];
    		printf("%s\n", getcwd(s, 100));
    		chdir(_simpleCommands[0]->_arguments[1]);
    		if (!_simpleCommands[0]->_arguments[1]){
    			chdir("/home");
    		}
    		printf("%s\n", getcwd(s, 100));
    		// exit(0);
		}
	
	if (_outFile || _catFile || _inputFile || _errFile){
		
		int outfd = -1;
	
		if (_outFile){
			outfd = open(_outFile, O_CREAT | O_TRUNC | O_WRONLY);
		}
		else{
			outfd = open(_catFile, O_CREAT | O_WRONLY | O_APPEND, 0777);
		}
		int infd = open(_inputFile, O_RDONLY);
		int errorfd = open(_errFile, O_WRONLY | O_APPEND, 0777);
		
		if ( outfd < 0 ) {
			outfd = defaultout;
		}
		if ( infd < 0 ) {
			infd = defaultin;
		}
		if ( errorfd < 0 ) {
		printf("not there");
			errorfd = defaulterr;
		}
		
		// Redirect output to the created utfile instead off printing to stdout 
		dup2( outfd, 1 );
		close( outfd );

		// Redirect input
		dup2( infd, 0 );
		close( infd );
		
		// Redirect output to file
		dup2( outfd, 1 );
		close( outfd );
		dup2( errorfd, 2 );
		// Redirect err
		//dup2( defaulterr, 2 );
		close( errorfd );
		
		excecute_command(0);
		
		dup2( defaultin, 0 );
		dup2( defaultout, 1 );
		dup2( defaulterr, 2 );

		// Close file descriptors that are not needed
		close( defaultin );
		close( defaultout );
		close( defaulterr );
	}
	else{
		parent_id = getpid();
		signal(SIGCHLD, handler);
		excecute_command(0);
		}
	}
	// Clear to prepare for next command
	clear();
	
	// Print new prompt
	prompt();

}
// Shell implementation

void
Command::prompt()
{
	char s[100];
	printf("myshell %s $ ", getcwd(s,100));
	fflush(stdout);
}


Command Command::_currentCommand;
SimpleCommand * Command::_currentSimpleCommand;

int yyparse(void);

int 
main()
{
	Command::_currentCommand.prompt();
	signal(SIGINT, signal_handler);
	yyparse();
	return 0;
}
