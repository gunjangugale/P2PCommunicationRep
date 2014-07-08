//This code is for the peer. The Peer enrolls itself with the server when it joins the network
//It informs the server about the files it has for sharing
//The peer can submit request for a file to the server
//Upon getting reply from the server, it can contact another peer and request to share the file
//If the peer gets file sharing request from another peer, it can share the file.

#include "stdafx.h"
#include "peer.h"
#include <sstream>
#include <iostream>
#include <fstream>

using namespace std;
char C_PEER_ID[200] = "";	
HANDLE h = CreateMutex(NULL, TRUE, NULL);

string ConnectToPeer(char PROCESS_TO_CONNECT[200], string FILE_REQUESTED, char PEER_PORT_NUMBER[200] );

DWORD WINAPI peer_to_peer(LPVOID lpParam) 
{
	char FILE_NAME[200] = "";
	char REQ_PEER[200] = "";
	SOCKET sock_CONNECTION = (SOCKET)lpParam; 
	recv(sock_CONNECTION, FILE_NAME, sizeof(FILE_NAME), NULL);
	recv(sock_CONNECTION, REQ_PEER, sizeof(REQ_PEER), NULL);
	cout << "\n\nPEER "<< REQ_PEER << "WANTS FILE " << FILE_NAME;

	string outproclist = "";
	outproclist.append("P"); 
	outproclist.append(C_PEER_ID);
	outproclist.append(FILE_NAME);			//FILE_NAME is file name		outproclist is filename for peer
	outproclist.append(".txt");

	ifstream myfile;
	myfile.open (outproclist);

	WaitForSingleObject(h, 100000);
	cout << "\n\nSENDING FILE " << FILE_NAME << " TO PEER " << REQ_PEER;
	char output[200] = "";
	if (myfile.is_open()) 
	{
		while (!myfile.eof()) 
		{
			myfile >> output;			
			send(sock_CONNECTION, output,sizeof(output),NULL);
		}
	}	
	ReleaseMutex(h);

	myfile.close();
	cout << "\n\nFILE SENT";
	closesocket(sock_CONNECTION); 					
	ExitThread(0);
	return 0;
}
//Creating threads
DWORD WINAPI receive_cmds(LPVOID lpParam) 
{
	DWORD thread; 
	
	SOCKET sock_LISTEN;
	SOCKET sock_CONNECTION;

	int portnum = *static_cast<int*>(lpParam);
	SOCKADDR_IN ADDRESS1;
	ADDRESS1.sin_addr.s_addr = inet_addr ("127.0.0.1");
	ADDRESS1.sin_family = AF_INET;
	ADDRESS1.sin_port = htons(portnum);
	int AddressSize1 = sizeof(ADDRESS1);

	sock_CONNECTION = socket(AF_INET,SOCK_STREAM,NULL);
	sock_LISTEN = socket(AF_INET,SOCK_STREAM,NULL);
	bind(sock_LISTEN, (SOCKADDR*)&ADDRESS1,sizeof(ADDRESS1));
	//listen for maximum no. of connections
	listen(sock_LISTEN, SOMAXCONN);		

	while(true)
	{						
		cout << "\n\nPEER: WAITING FOR INCOMING CONNECTION...";	
		sock_CONNECTION = accept(sock_LISTEN,(SOCKADDR*)&ADDRESS1,&AddressSize1);
		cout << "\n\nCONNECTION FOUND\n" << endl;	
	    //Creating thread for communication among peers
		CreateThread(NULL, 0,peer_to_peer,(LPVOID)sock_CONNECTION, 0, &thread); 
	}

	CloseHandle(h);
	closesocket(sock_CONNECTION); 
	ExitThread(0);
	return 0;
}

void main()
{
	long SUCCESSFUL;
	WSAData WinSockData;
	WORD DLLVERSION;
	DWORD thread; 
	DLLVERSION = MAKEWORD(2,1);
	SUCCESSFUL = WSAStartup(DLLVERSION, &WinSockData);
	SOCKADDR_IN ADDRESS;
	SOCKET sock;
	sock = socket(AF_INET,SOCK_STREAM,NULL);
	ADDRESS.sin_addr.s_addr = inet_addr ("127.0.0.1");
	ADDRESS.sin_family = AF_INET;
	ADDRESS.sin_port = htons(6013);
	int AddressSize = sizeof(ADDRESS);
	
	string RESPONSE; 
	string FILE_REQUESTED = "";
	string FILE_REQUEST = "";
	cout << "\nCLIENT : DO YOU WANT TO CONNECT TO A SERVER? (Y/N)"; 
	cin >> RESPONSE;	
	if ((RESPONSE == "n")|| (RESPONSE == "N"))
		cout << "\nOK I AM QUITING...";
	else if ((RESPONSE == "y") || (RESPONSE == "Y"))
		{
			connect(sock,(SOCKADDR*)&ADDRESS,sizeof(ADDRESS));
			
			char buf[200] = "";	
			char FILELIST[200] ="";
			cout << "\nWHAT FILES DO YOU HAVE? ";
			cin >> buf;				
			send(sock,buf,sizeof(buf),NULL);
			char PORT_NUM_PEER[200] = "";
			cout << "\nENTER THE PORT NUMBER: ";			
			cin >> PORT_NUM_PEER;										
			SUCCESSFUL = send(sock, PORT_NUM_PEER,sizeof(PORT_NUM_PEER),NULL);
			SUCCESSFUL = recv(sock, C_PEER_ID, sizeof(C_PEER_ID), NULL);
			cout << "\n\nPEER ID ASSIGNED BY SERVER IS : " << C_PEER_ID;
			int portnum = atoi (PORT_NUM_PEER);	
			int *id = new int(portnum);
			//Creating thread for each connection to server
			CreateThread(NULL, 0,receive_cmds,id, 0, &thread); 
			
			int flag = 1;
			while (flag == 1)
			{
				     int ENTER_OPTION = 0; 
				     cout << "\n\nENTER \n1 TO RECEIVE FILE\n2 TO EXIT THE NETWORK";
				     cin >> ENTER_OPTION;	
				if (ENTER_OPTION == 1)
				{
					stringstream ss4;
					string linebuf;
					ss4 << buf;
					ss4 >> linebuf;
					cout << "\n\nYOU HAVE FILE(S) " << buf << ".WHAT FILES DO YOU NEED?";
					cin >> FILE_REQUESTED;
					unsigned found=linebuf.find(FILE_REQUESTED);
					if (found!=std::string::npos)					
					{
						cout << "\n YOU ALREADY HAVE FILE " << FILE_REQUESTED;					   
					}
					else
					{
						stringstream ss1;
						string line;
						ss1 << C_PEER_ID;
						ss1 >> line;
						string newline = "PEER" + line +  "WANTSFILE"+ FILE_REQUESTED;							
						const char * FILE_REQUIRE = newline.c_str ();										
						char FILE_REQUIRE_CHAR[200] = "";
						char C_PROCESS_FOUND[200] = "";
						strncpy (FILE_REQUIRE_CHAR, FILE_REQUIRE, sizeof FILE_REQUIRE_CHAR);
						//Sending request to the server for file search among peers
						SUCCESSFUL = send(sock,FILE_REQUIRE,sizeof(FILE_REQUIRE_CHAR),NULL);
						//Receiving response from server, which peer has file
						SUCCESSFUL = recv(sock, C_PROCESS_FOUND, sizeof(C_PROCESS_FOUND), NULL);

						stringstream ss7;
						string msgstr;
						ss7 << C_PROCESS_FOUND;
						ss7 >> msgstr;
						unsigned found=msgstr.find("NO");
						if (found!=std::string::npos)
						{
							cout << "\n" << C_PROCESS_FOUND;
						}
						else
						{
							cout << "\n FILE " << FILE_REQUESTED << " IS WITH PEER " << C_PROCESS_FOUND;
							char PEER_PORT_NUMBER[200] = "";
							char PEER_TO_CONNECT[200] = "";
							cout << "\n\nWHAT PEER YOU WANT TO CONNECT TO? ";
							cin >> PEER_TO_CONNECT;
							SUCCESSFUL = send(sock,PEER_TO_CONNECT,sizeof(PEER_TO_CONNECT),NULL);
							SUCCESSFUL = recv(sock, PEER_PORT_NUMBER, sizeof(PEER_PORT_NUMBER), NULL);
							cout << "\n\nYOU ARE CONNECTED TO PEER  " << PEER_TO_CONNECT;
							cout << "\n\nDOWNLOADING FILE " << FILE_REQUESTED ;
							string  output = ConnectToPeer(PEER_TO_CONNECT, FILE_REQUESTED, PEER_PORT_NUMBER );	
							//linebuf = output;
							//cout << linebuf;
							//const char * buf = linebuf.c_str ();	
							//giving update to sever abot new file  with peer
							const char * MESSAGE_TO_SERVER = output.c_str ();										
							char MESSAGE_TO_SERVER_COPY[200] = "";
							strncpy (MESSAGE_TO_SERVER_COPY, MESSAGE_TO_SERVER, sizeof MESSAGE_TO_SERVER_COPY);
							SUCCESSFUL = send(sock,MESSAGE_TO_SERVER,sizeof(MESSAGE_TO_SERVER_COPY),NULL);
							SUCCESSFUL = recv(sock, buf, sizeof(buf), NULL);
							cout << "\nFILE DOWNLOAD COMPLETE";
						}
			       	}
				}

				//Exit the network by deleting Peer's data
				else if (ENTER_OPTION == 2)
				{
					flag = 0;
					string sendtosrv = "PEER";
					sendtosrv.append(C_PEER_ID); 
					sendtosrv.append("DELETE"); 
					const char * SEND_TO_SERVER = sendtosrv.c_str ();										
					char MESSAGE8[200] = "";
					strncpy (MESSAGE8, SEND_TO_SERVER, sizeof MESSAGE8);
					SUCCESSFUL = send(sock,SEND_TO_SERVER,sizeof(MESSAGE8),NULL);
					exit(1);
				}
				else 
					cout << "\n INVALID ENTER_OPTION";
			}
		}
	
		else
			cout << "\n\nTHAT WAS A WRONG RESPONSE!!!";

	closesocket(sock); 	
	WSACleanup(); 
	getchar();
}

//Connection to requested Peer by another Peer
string ConnectToPeer(char PEER_TO_CONNECT[200], string FILE_REQUESTED, char PEER_PORT_NUMBER[200] )
{	
	SOCKADDR_IN ADDRESS;
	SOCKET sock_peer;
	sock_peer = socket(AF_INET,SOCK_STREAM,NULL);
	int portnum = atoi(PEER_PORT_NUMBER);					
	ADDRESS.sin_addr.s_addr = inet_addr ("127.0.0.1");
	ADDRESS.sin_family = AF_INET;
	ADDRESS.sin_port = htons(portnum);
	connect(sock_peer,(SOCKADDR*)&ADDRESS,sizeof(ADDRESS));
	const char * MESSAGE = FILE_REQUESTED.c_str ();
	send(sock_peer,MESSAGE,sizeof(MESSAGE),NULL);
	send(sock_peer,C_PEER_ID,sizeof(C_PEER_ID),NULL);

	string outproclist = "";
	outproclist.append("P"); 
	outproclist.append(C_PEER_ID);
	outproclist.append(MESSAGE);			//MESSAGE is file name		outproclist is filename for peer
	outproclist.append(".txt");	

	ofstream myfile;
	myfile.open (outproclist);

	char output[200] = "";
	if (myfile.is_open()) 
	{		
		while((recv(sock_peer, output,sizeof(output),NULL)) > 0)
		{			
			myfile << output;
			myfile << " ";			
		}					
	}	

	myfile.close();
	closesocket(sock_peer);	

	outproclist.append("UPDATE");		//outproclist has filenameupdate

	return outproclist;
}






