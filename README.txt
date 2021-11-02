# Implementation of Reliability in UDP File transfer--CSCI 5273#
The project is a part of CSCI 5273 001: Network Systems.
UDP is a connectionless transmission model with a minimum of protocol mechanism.
It has no handshaking dialogues and no mechanism of acknowledgements (ACKs).
Therefore, every packet delivered by using UDP protocol is not guaranteed to be received, but UDP avoids unnecessary delay, thanks to its simplicity.
In this project we have implemented reliability by using Stop-and-Wait protocol.

## Reliability
Reliability is implemented using a STOP-AND-WAIT protocol that uses frame acknowledgements after every frame has been sent.
Also included are a resend mechanism and timeouts on both client and server.

## Commands
Five commands have been implemented: 
* get [File name]    :-The server transmits the requested file to the client
* put [File name]    :-The server receives the transmitted file by the client and stores it locally
* delete [File name] :-The server delete file if it exists. Otherwise do nothing
* ls                 :-The server should search all the files it has in its local directory and send a list of all these files to the client.
* exit               :-The server should exit gracefully.

## Instructions
The folder consists of a makefile and two directories named client_dir and server_dir, consisting of their respective .c file.
The makefile stores object files in *obj/*.
```
make
```
Once the compilation is done, the server and the client executables can be run in the following manner:
### Server
```
./filepath/run/server [Port Number] 

Example: $./server 5001
```
*Port Number* must be greater than 5000.
### Client
```
./filepath/run/client [Server IP Address] [Server Port Number]

Example: $./client hostname 5001
```
### Running commands on the client
**NOTE**: In the above commands, 'filepath' must be replaced by the path on your system, based on your current directory. This is especially important for the *LS* command as the server lists its current directory based on where it is running (where it was called from).

### Creating large files for testing
The following command can be run to create an arbitrary file of a size of your choosing.
```
dd if=/dev/zero of=testfile bs=1024 count=102400
```
The variables *bs* and *count* can be modified to create a file size as large or small as you want.

## Authors
* Yeshu Sharma

## License
[MIT](https://choosealicense.com/licenses/mit/)
