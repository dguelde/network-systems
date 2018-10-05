Donovan Guelde
CSCI 5273 PA1

PACKAGE CONTENTS:

Package unzips into two separate folders, ./server/ and ./client/  Each folder contains a makefile and c files necessary for compilation.  Executing the makefile will compile server.c into ./server, and client.c into ./client.

USAGE:

Usage is as specified in the writeup, server requires one command line argument for port, and client requires the first command line argument for server’s IP address, and the second command line argument for server’s port number. Server should be launched before client.

Server has very little(if any) output to the terminal, and takes no input from the user once launched appropriately.

Client takes input from the user via the terminal, sends commands to the server, and responds appropriately.  A simple main menu is displayed each time the loop executes, so the user always sees available commands.  Possible commands are as specified in the writeup, and are limited to get <filename>, put <filename>, delete <filename>, ls, and exit.

COMMANDS:
	get <filename>:  Client requests that server send <filename>.  This file must be located in server’s local directory, and will be saved in client’s local directory.  If server cannot open the file, client will notify the user that the download has failed.  User can then enter another command.

	put <filename>:  Client uploads the file to server.  If client cannot open the file, user will be notified and the main menu will be displayed, allowing user to enter another command.  The file must be located in client’s local directory, and will be saved in server’s local directory.

	ls:  Server will send a series of messages to the client, where each message contains the name of a file saved in server’s local directory.  After all filenames have been sent, server sends a ‘done’ message so the client (and the user) know that the complete file listing has been sent.  These messages are printed on separate lines in the client window.

	delete <filename>: Server will delete the file from its local directory.

	exit:  Server will acknowledge the exit command from client and close down.  Client will wait for this acknowledgement.  If the acknowledgement is received, client will also close.  If the client does not receive the acknowledgement, it will timeout and resend the exit command to the server and wait.  After ten timeouts, client assumes that server has shut down but acknowledgement was lost.  Client will now close.


DETAILS OF OPERATION:

UPON LAUNCH:
Upon launch, server opens a socket, binds it, and listens for incoming messages.  The first message received is compared to a predetermined message to perform a simple handshake.  If this handshake message is not received, the server will continue to check incoming packets until it is received.  Upon successful handshake, the server acknowledges this message, and enters a loop where it receives and processes incoming messages.  Server will respond to the commands specified in the assignment writeup (get, put, delete, ls, and exit).  Capitalization is not ignored, so GET will not execute the <get> function. If a message is received that does not correlate to any of these commands, server will simply echo that message back to the client.

Upon launch, client will open a socket and attempt to connect to the IP address and port specified by the user. The first message sent is the predetermined handshake message.  The server will wait for this message to be acknowledged, and if timeout occurs (after one second), it will resend the handshake.  Upon successful handshake with the server, the client enters a loop to receive user input, send the command to the server, and process the command appropriately.  If the client cannot complete the handshake after ten timeouts, client will inform the user, and exit.

MAIN LOOP:
	Client’s main loop first displays the menu, takes user command, and sends the command to the server.  If user input matches one of the defined commands, the client will call an appropriate function to process messages from the server to complete the user request.  Upon completion, client re-enters the main loop.

	Server executes in a similar fashion.  In the main loop, server accepts a packet from client, and if the message matches one of the pre-defined commands, server calls the appropriate function to carry out the received command.  If there is no matching command, server simply echoes this message back to the client.

COMMANDS:
	get <filename>: Client sends file to server.  To ensure reliability, I implemented a simple ‘stop-and-wait’ protocol.  Client reads files in chunks (maximum size 500 bytes), and sends these packets to the server.  Each packet is pre-pended with the packet number.  The server waits to receive a packet in blocking mode.  When a packet is received, the server checks the packet number, and if the packet is the next packet expected, it is written to the file and acknowledged.  If the packet is not expected, the server sends an acknowledgement with the expected packet number. When an ACK is received by the client, it checks the packet number in the ACK.  If this matches the number of the packet just sent, the next packet in order will be sent.  Since the server waits in blocking mode, it will not be affected by slow network speeds, etc.  Since the client has a timeout enabled on recvfrom, it will continue to resend packets until ACK is received. Tested over a remote connection, with a built-in random chance of both ACKs and packets not being sent to simulate poor network performance.  Transfer is slow but reliable.

	get <filename>: Basically the same as put, but with roles reversed.  Server is still in blocking mode, so will not send a packet until an ACK is received.  Client has a timeout, so is able to resend ACKs if one is lost.  This ensures file download accuracy.

	ls: Server uses the popen() function to launch a child thread, which executes ‘ls’ and piped the result back to the parent thread (server).  These results are then sent to client.  No reliability is built into this function except for the client resending the ’ls’ command to the server until this command is acknowledged.

	delete: Server attempts to delete the file specified.  If successful, the client is notified of success.  Likewise, the client is notified if the delete fails.

	exit: If server receives the exit command, it will ACK this command, then close.  If the client receives this ACK, user is notified, and client closes.  If no ACK is received, client will wait for 10 timeouts (10 seconds total), resending the exit command each time.  If no ACK is received, it can be assumed that server sent an ACK and shut down, but the ACK was lost.  Client then closes.
	

