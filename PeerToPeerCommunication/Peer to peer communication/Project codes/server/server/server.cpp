//This code is for the server. When the peer joins the network, the server adds the peer along with the details of files
//it has for sharing. Server helps peer find files it is looking for
//When the peer receives a new file, the server updates its entry table
//Server removes the peer's entry from the table when the peer exits the network

#include "stdafx.h"
#include "server.h"
#include <sstream>

using namespace std;
long SUCCESSFUL;
int procnum = 0;
int totalpeer = 0;
string procmat [9][3];

string NewPeer(string port,char recvfiles[200]);
string FindFile( string filestr);
string FindPortNum(char ProcessName[200]);
void DeletePeer(string delpeer);
void DisplayData ();
string UpdateFile (string ProcessName, string FileName);

DWORD WINAPI receive_cmds(LPVOID lpParam) 
{	
	SOCKET sock_CONNECTION = (SOCKET)lpParam; 
	struct sockaddr_in address; 
	
	int size = sizeof(SOCKADDR_IN);
	int addressLength = sizeof(address);    

	//Receive names of files that this peer holds
	char recvfiles[200] = "";
	SUCCESSFUL = recv(sock_CONNECTION, recvfiles, sizeof(recvfiles), NULL);	  	
	
	//Receive peer's port number
	char recvportnum[200] = "";
	SUCCESSFUL = recv(sock_CONNECTION, recvportnum, sizeof(recvportnum), NULL);
		
	string port;   
	stringstream ss4;		
	ss4 << recvportnum;
	ss4 >> port;	
	
	//Create entry for newly added peer in the data file and data matrix
	//Assign an id to the peer and send it to the peer
	string processid = NewPeer(port,recvfiles);	
	const char * procid = processid.c_str();	
	SUCCESSFUL = send(sock_CONNECTION, procid,sizeof(procid),NULL);		
	cout << "\n\nPEER " << procid << " HAS JOINED THE NETWORK";

	//Receive commands from the peer and process them
	int flag = 1;
	while(flag == 1)
	{
		char peercmd[200] = "";
		SUCCESSFUL = recv(sock_CONNECTION, peercmd, sizeof(peercmd), NULL);		
		string msgstr = "";
		stringstream ss2;
		ss2 << peercmd;
		ss2 >> msgstr;	
		
		//Check if the peer wants to find a file, or update the file list, or wants to exit the network
		string procfound = "";
		unsigned found=msgstr.find("WANTS");
		if (found!=std::string::npos)
		{			
			string filestr = msgstr.substr(14,2);
			string procstr = msgstr.substr(4,1);
			//Find which peers have the file
			procfound = FindFile( filestr);	
			cout << "\n\nPEER " << procstr << " WANTS FILE " << filestr;
			//If no peer has this file, let the peer know
			if (procfound.empty())
			{
				SUCCESSFUL = send(sock_CONNECTION, "NO PEER HAS THIS FILE",200,NULL);
			}
			else
			{
				//send the list of peers to this peer
				const char * cprocfound = procfound.c_str();				
				char proclist[200] = "";
				strncpy (proclist, cprocfound, sizeof proclist);
				SUCCESSFUL = send(sock_CONNECTION, cprocfound,sizeof(proclist),NULL);

				//peer sends what peer it wants to connect to
				//send the that peer's port number to this peer
				char procconnect[200] = "";
				SUCCESSFUL = recv(sock_CONNECTION, procconnect, sizeof(procconnect), NULL);
				string portnum = FindPortNum(procconnect);
				const char * cportnum = portnum.c_str();
				SUCCESSFUL = send(sock_CONNECTION, cportnum,sizeof(proclist),NULL);
			}
		}
		//check if the peer wants to update its file list
		found=msgstr.find("UPDATE");
		if (found!=std::string::npos)
		{			
			string updatepro = msgstr.substr(1,1);
			string updatefile = msgstr.substr(2,2);
			cout << "\n\nPEER " << updatepro << " NOW HAS FILE " << updatefile;
			//update the data matrix and data file
			string newfilelist = UpdateFile(updatepro, updatefile);
			const char * filelist = newfilelist.c_str();
			SUCCESSFUL = send(sock_CONNECTION, filelist,200,NULL);
		}
		//check if the peer wants to exit the network
		found=msgstr.find("DELETE");
		if (found!=std::string::npos)
		{
			string delpeer = msgstr.substr(4,1);
			cout << "\n\nPEER " << delpeer << " IS EXITING THE NETWORK";
			//delete the peer's data from data matrix and data file
			DeletePeer(delpeer);
			flag = 0;
			closesocket(sock_CONNECTION); 
			ExitThread(0);
		}
	}
	return 0;
}

void main()
{	
	WSAData WinSockData;
	WORD DLLVERSION;
	DWORD thread; 

	DLLVERSION = MAKEWORD(2,1);
	SUCCESSFUL = WSAStartup(DLLVERSION, &WinSockData);
	SOCKADDR_IN ADDRESS;
	int AddressSize = sizeof(ADDRESS);

	SOCKET sock_LISTEN;
	SOCKET sock_CONNECTION;

	sock_CONNECTION = socket(AF_INET,SOCK_STREAM,NULL);
	ADDRESS.sin_addr.s_addr = inet_addr ("127.0.0.1");
	ADDRESS.sin_family = AF_INET;
	ADDRESS.sin_port = htons(6013);		

	sock_LISTEN = socket(AF_INET,SOCK_STREAM,NULL);
	bind(sock_LISTEN, (SOCKADDR*)&ADDRESS,sizeof(ADDRESS));
	listen(sock_LISTEN, SOMAXCONN);		

	cout << "WELCOME TO THE P2P NETWORK!";	
	
	while(true)
	{		
		sock_CONNECTION = accept(sock_LISTEN,(SOCKADDR*)&ADDRESS,&AddressSize);		
		CreateThread(NULL, 0,receive_cmds,(LPVOID)sock_CONNECTION, 0, &thread); 		
	}

	closesocket(sock_LISTEN); 	
	WSACleanup(); 
}

//Adds new peer to the network
string  NewPeer(string port,char recvfiles[20])
{
		//procnum stores the total number of processes in the network
		procnum++;
		totalpeer++;
						
		string filelist = "";		
		stringstream ss1;		
		ss1 << recvfiles;
		ss1 >> filelist;	
		int index = 0;

		//if some peer has left the network, then its peer id can be used for another peer
		if((procnum) - (totalpeer) > 0)
		{
			for(int i = 1; i<= procnum; i++)
			{
				if (procmat [i][0] == "")		//find any empty position 
				{
					index = i;
					procnum--;
					i = procnum + 1;
				}
			}
		}
		else
		{
			index = procnum;
		}

		stringstream ss;
		ss << index;
		string peerid = "";
		peerid = ss.str();

		//procmat stores data for all the peers in the network
		//the data includes peer's id, port num, and file list
		procmat [index][0] = peerid;				//id
		procmat [index][1] = port;					//port number
		procmat [index][2] = filelist;				//file list

		DisplayData ();

		return peerid; 
}

//Finds list of peers which have the file that the connected peer wants
string FindFile(string filestr)
{	
	string outproclist = "";
	
	for (int i=1; i<=procnum; i++)
	{
		string proclist = procmat [i][2];
		unsigned found=proclist.find(filestr);
		if (found!=std::string::npos)
		{
			outproclist.append("P"); 
			outproclist.append(procmat[i][0]); 
			outproclist.append(","); 
		}		
	}

	return outproclist;
}

//Finds the port number of the requested peer
string FindPortNum(char ProcessName[200])
{

	stringstream ss2;
	int index = 0;
	ss2 << ProcessName [1];
	ss2 >> index;	
	
	return procmat [index][1];
}

//Delete the peer from network
void DeletePeer(string delpeer)
{
	stringstream ss5;
	int index = 0;
	ss5 << delpeer;
	ss5 >> index;

	//Remove the peer's entry from data matrix
	for (int i=0; i<=2; i++)
	{
		procmat[index][i] = "";
	}
	totalpeer--;
	DisplayData ();
}

//Display the list of peers and their data currently in the network
void DisplayData ()
{
	cout << "\n\n  PEER NAME   PORT NO.  FILES";
	for (int i = 1; i<= procnum ; i++)
	{
		if (procmat[i][0] != "")
		{
			cout << "\n";
			for (int j=0; j <= 2;j++)
			{
				cout << "\t" << procmat[i][j];
			}
		}
	}

}

//Update the file list for the peer
string UpdateFile (string ProcessName, string FileName)
{
	stringstream ss;
	int index = 0;
	ss << ProcessName;
	ss >> index;

	string FileList = procmat[index][2] ;
	FileList.append(",");
	FileList.append(FileName);
	procmat[index][2] = FileList;

	DisplayData ();
	return FileList;
}



