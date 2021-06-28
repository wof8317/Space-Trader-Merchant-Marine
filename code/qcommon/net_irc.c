

#ifndef DEDICATED

#ifdef _MSC_VER
#pragma warning( disable : 4206 ) //nonstandard extension: empty translation unit
#endif

#ifdef USE_IRC

#include "q_shared.h"
#include "qcommon.h"

typedef enum {
	RPL_WELCOME = 1,
	RPL_YOURHOST,
	RPL_CREATED,
	RPL_MYINFO,
	RPL_BOUNCE,
	RPL_AWAY = 301,
	RPL_USERHOST,
	RPL_ISON,
	RPL_UNAWAY = 305,
	RPL_NOAWAY,
	RPL_LIST = 322,
	RPL_LISTEND,
	ERR_CANNOTSENDTOCHAN=404,
	ERR_NOMOTD = 422,
	ERR_NICKNAMEINUSE = 433,
	ERR_NICKCOLLISION = 436,
	ERR_ALREADYREGISTRED = 462,
} irc_response_t;

extern	cvar_t	*cl_authserver;
extern	cvar_t	*cl_ircport;
extern	cvar_t	*sv_ircchannel;
extern	cvar_t	*sv_hostname;
extern	cvar_t	*sv_mapname;

#define IRC_PORT cl_ircport->integer
#define IRC_SERVER cl_authserver->string
static netadr_t irc_server;

static int irc_connected=qfalse;
static int irc_registered=qfalse;

#ifndef DEDICATED
extern void	CL_ServerAddToRefreshList( const char *info, const char *channel );
#endif

extern qboolean Sys_GetPacket ( netadr_t *net_from, msg_t *net_message );
extern qboolean Sys_GetPacketData( netadr_t *net_from, msg_t *net_message );
extern int NET_IPSocket (char *net_interface, int port, netadrtype_t type);

void Net_IRC_OpenNetworkPort(char *host){
	char *addr;
	if ( host ) {
		netadr_t client;
		addr = strstr(host, "@")+1;
		if (NET_StringToAdr( addr, &client ) )
		{
			client.port = BigShort(Cvar_VariableIntegerValue("net_port"));
			NET_OutOfBandPrint( NS_SERVER, client, "poke" );
		}
	}
}
/*void Net_IRC_QueryRemotePort(){
	netadr_t to;
	NET_StringToAdr( va("%s:%d", IRC_SERVER, PORT_SERVER ), &to );
	NET_OutOfBandPrint( NS_SERVER, to, "getmylocalport" );
}*/
void Net_IRC_HandleResponse(char *line)
{
	char *cmd;
	const char *text_p;
	int msgnum;
	text_p = line;
	Com_Printf("IRC: Raw: [%s]\n", line);
	if (line[0] == ':') {
		char *host, *msg;
		host = COM_Parse(&text_p);	// <nick>!<user>@ip
		msg = COM_Parse(&text_p);
		msgnum = atoi(msg);
		if (msgnum == 0)
		{
			switch(SWITCHSTRING(msg))
			{
				case CS('N','O','T','I'): //NOTICE
					if (irc_registered == qfalse) {
						cvar_t *name = Cvar_Get("name", "UnnamedPlayer", CVAR_USERINFO | CVAR_ARCHIVE);
						
						Net_IRC_RegisterUser(name->string, NULL);
					}
					Com_Printf("IRC: [NOTICE]: %s\n", text_p);
					break;
				case CS('J','O','I','N'): // JOIN
					if (com_sv_running->integer!= 0) {

						char * channel = COM_Parse(&text_p);
						//ignore the : in front of the channel name
						channel++;

						//	someone is joining this server's channel
						if ( !Q_stricmp( channel, sv_ircchannel->string ) ) {
							cvar_t *sv_ircnick = Cvar_Get("sv_ircnick", "na", CVAR_ROM);
							char *end_of_nick = strstr(host+1, "!");
							char nick[256];
							Q_strncpyz(nick, host+1, end_of_nick - host);
							if ( !Q_stricmp( nick, sv_ircnick->string) ) { // if we are the ones joining our channel, then set the topic
								char *addr = strstr(host, "@")+1;
								Net_IRC_SendMessage( va("TOPIC %s :addr\\%s\\hostname\\%s\\mapname\\%s", sv_ircchannel->string,addr, sv_hostname->string, sv_mapname->string ) );
							}
							Net_IRC_OpenNetworkPort(host);
						}

					}
					break;
			}
		} else {
			switch(msgnum) {
				case RPL_WELCOME:
				case RPL_YOURHOST:
				case RPL_CREATED:
				case RPL_BOUNCE:
					break;
				case RPL_MYINFO:
					irc_registered = qtrue;
					Com_Printf("Registered!\n");
					//Net_IRC_JoinChannel("#consolespam");
					Net_IRC_JoinChannel( Cvar_VariableString( "sv_ircchannel" ) );
					break;

				case RPL_LIST:	//"<channel> <# visible> :<topic>"
					{
#ifndef DEDICATED
						if ( !com_sv_running->integer ) {

							const char *channel;

										COM_Parse(&text_p); // username
							channel =	COM_Parse(&text_p);	// channel

							if ( !Q_strncmp( channel, "#server-", 8 ) ) {
								COM_Parse(&text_p); // num users
								COM_Parse(&text_p); // channel mode
								text_p++;

								CL_ServerAddToRefreshList( text_p, channel );
							}
						}
#endif
					}
					break;
				case RPL_LISTEND:
					break;
				case ERR_CANNOTSENDTOCHAN:
				case ERR_NOMOTD:
				case ERR_ALREADYREGISTRED:
					break;

				case ERR_NICKNAMEINUSE:
					{
						char *nick = va("%s_%d", Cvar_VariableString( "name" ), Q_rand() );
						Cvar_Set("sv_ircnick", nick);
						Net_IRC_SendMessage( va("NICK %s", nick) );
					}
					break;

				default:
					if ( msgnum > 400 ) {
						Com_Printf( "irc error: %d\n", msgnum ); 
					}
					break;
			}
		}
		// Server messages?
	} else {
		cmd = COM_Parse(&text_p);
		switch(SWITCHSTRING(cmd))
		{
			case CS('P','I','N','G'):
			{
				char *host;
				host = COM_Parse(&text_p);
				Net_IRC_SendMessage(va("PONG %s", host));
				break;
			}
			default:
				Com_Printf("IRC: Unknown server message: [%s %s]\n", cmd, text_p);
				break;
		}
	}
}

void Net_IRC_CleanString(char *msg)
{
	int i;
	for(i=0; i < strlen(msg); i++)
	{
		switch(msg[i]){
			case '\r':
			case '\n':
				msg[i] = ' ';
				break;
		}
	}
}

void Net_IRC_SendMessage(const char *msg)
{
	char buf[512];
	//We are going to toss a \r\n to the end, so only copy the first 510 bytes
	Q_strncpyz(buf, msg, 510);
	Net_IRC_CleanString(buf);
	Q_strcat(buf, 512, "\r\n");
	if (irc_server.socket != 0)
	{
		NET_SendPacket(0, strlen(buf), buf, irc_server);
	}
}

void Net_IRC_ReadMessage(msg_t *netmsg)
{
	char buf[512]; //irc max line length is 512
	int i, offset;
	for (i=0,offset=0; i < netmsg->cursize-1; i++)
	{
		if (netmsg->data[i] == '\r' && netmsg->data[i+1] == '\n'){
			if ( (i - offset+1) < sizeof(buf) )
			{
				Q_strncpyz(buf, netmsg->data + offset, i - offset+1);
				Net_IRC_HandleResponse(buf);
				//set our offset to i + the \r\n
				offset = i+2;
				// increment i for the \r, when the for loop wraps it will
				// increment again for the \n
				i++;
			} else {
				Com_Error(ERR_DROP, "Invalid packet");
			}
		}
	}
}

//A return of qtrue means the message was able to be sent, not that you
//were actually able to be registered successfully
qboolean Net_IRC_RegisterUser(char *nick, char *pass)
{
	if (irc_connected)
	{
		if ( !irc_registered )
		{
			if (pass)
				Net_IRC_SendMessage( va("PASS %s", pass));
		
			Cvar_Set("sv_ircnick", nick);
			Net_IRC_SendMessage( va("NICK %s", nick));
			Net_IRC_SendMessage( va("USER %s NA NA :NA", nick) );
			return qtrue;
		}
		return qfalse;
	}
	return qfalse;
}
void Net_IRC_ListServers()
{
	if ( irc_connected && irc_registered )
		Net_IRC_SendMessage("LIST");
}


//A return of qtrue means the message was able to be sent, not that you
//were actually able to join the channel
qboolean Net_IRC_JoinChannel(const char *channel)
{
	if (irc_connected && irc_registered) {
		Net_IRC_SendMessage(va("JOIN %s", channel));
		return qtrue;
	}
	return qfalse;
}

void Net_IRC_SendChat(const char *channel, const char *message)
{
	if (irc_connected && irc_registered)
	{
		char msg[ 2048 ];
		Com_sprintf( msg, sizeof(msg), "PRIVMSG %s : %s", channel, message );
		Net_IRC_SendMessage( msg );
	}
}

void Net_IRC_Init()
{
	if (irc_server.socket == 0 ) {
		irc_server.type = NA_TCP;
		irc_server.socket = NET_IPSocket(IRC_SERVER, IRC_PORT, NA_TCP);
		if (irc_server.socket > 0)
		{
			irc_connected = qtrue;
		} else {
			Com_Error(ERR_DROP, "Unable to create socket");
		}
	} else {
		Com_Error(ERR_DROP, "IRC Already Initialized");
	}
}

void Net_IRC_Pump()
{
	char	buf[MAX_MSGLEN];
	msg_t	netmsg;
	
	MSG_Init( &netmsg, buf, sizeof( buf ) );
	if ( Sys_GetPacketData ( &irc_server, &netmsg ) )
	{
		Net_IRC_ReadMessage(&netmsg);
	}
}

void Net_IRC_Shutdown(const char *msg)
{
	NET_Shutdown(irc_server.socket);
	irc_registered = qfalse;
	irc_connected = qfalse;
	irc_server.socket = 0;
}

#endif

#endif