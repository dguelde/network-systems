Compile and Execute:
	Written in C, compiles without error using gcc-7 under OS X.  <gcc-7 PA2.c>

Usage:
	Once server reads the ws.conf file, it takes no input from the user.

	Upon execution, server attempts the system call <<system("ipconfig getifaddr en0”);>>  If executed in OS X, this will print the server’s IP address for user’s convenience.

	Server will handle both HTTP/1.0 and HTTP/1.1 requests.  Keepalive time is obtained from ws.config file.

	Server supports only the GET request.