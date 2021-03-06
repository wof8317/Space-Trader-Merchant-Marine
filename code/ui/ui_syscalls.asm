code

equ	trap_Error								-1
equ	trap_Print								-2
equ	trap_Milliseconds						-3
equ	trap_Cvar_Set							-4
equ	trap_Cvar_VariableValue					-5
equ	trap_Cvar_VariableStringBuffer			-6
equ trap_Cvar_VariableInt					-7
equ	trap_Cvar_SetValue						-8
equ	trap_Cvar_Reset							-9
equ	trap_Cvar_Create						-10
equ	trap_Cvar_InfoStringBuffer				-11
equ	trap_Argc								-12
equ	trap_Argv								-13
equ trap_ArgvI								-14
equ	trap_Cmd_ExecuteText					-15
equ	trap_FS_FOpenFile						-16
equ	trap_FS_Read							-17
equ	trap_FS_Write							-18
equ	trap_FS_FCloseFile						-19
equ	trap_FS_GetFileList						-20
equ	trap_R_RegisterModel					-21
equ	trap_R_RegisterSkin						-22
equ trao_R_RegisterShader					-23
equ	trap_R_RegisterShaderNoMip				-24
equ	trap_R_ClearScene						-25
equ trap_R_BuildPose						-26
equ trap_R_BuildPose2						-27
equ trap_R_BuildPose3						-28
equ trap_R_ShapeCreate						-29
equ trap_R_ShapeDraw						-30
equ trap_R_GraphDraw						-31
equ trap_R_ShapeDrawMulti					-32
equ	trap_R_AddRefEntityToScene				-33
equ	trap_R_AddPolyToScene					-34
equ	trap_R_AddLightToScene					-35
equ	trap_R_RenderScene						-36
equ	trap_R_SetColor							-37
equ	trap_R_DrawStretchPic					-38
equ trap_R_DrawSprite						-39
equ trap_R_DrawText							-40
equ trap_R_FlowText							-41
equ trap_R_GetFont							-42
equ trap_R_RenderRoundRect					-43
equ	trap_UpdateScreen						-44
equ	trap_CM_LerpTag							-45
equ	trap_CM_LoadModel						-46
equ	trap_S_RegisterSound					-47
equ	trap_S_StartLocalSound					-48
equ	trap_Key_KeynumToStringBuf				-49
equ	trap_Key_GetBindingBuf					-50
equ	trap_Key_SetBinding						-51
equ	trap_Key_IsDown							-52
equ	trap_Key_GetOverstrikeMode				-53
equ	trap_Key_SetOverstrikeMode				-54
equ	trap_Key_ClearStates					-55
equ	trap_Key_GetCatcher						-56
equ	trap_Key_SetCatcher						-57   
equ trap_Key_GetKey							-58  
equ	trap_GetClipboardData					-59   
equ	trap_GetGlconfig						-60
equ	trap_GetClientState						-61
equ trap_GetClientNum						-62
equ	trap_GetConfigString					-63
equ trap_UpdateGameState					-64
equ	trap_LAN_GetPingQueueCount				-65
equ	trap_LAN_ClearPing						-66
equ	trap_LAN_GetPing						-67
equ	trap_LAN_GetPingInfo					-68
equ	trap_Cvar_Register						-69
equ trap_Cvar_Update						-70
equ trap_MemoryRemaining					-71
equ trap_R_RegisterFont						-72
equ trap_R_GetFonts							-73
equ trap_R_ModelBounds						-74
equ trap_PC_AddGlobalDefine					-75
equ	trap_PC_LoadSource						-76
equ trap_PC_FreeSource						-77
equ trap_PC_ReadToken						-78
equ trap_PC_SourceFileAndLine				-79
equ trap_S_StopBackgroundTrack				-80
equ trap_S_StartBackgroundTrack				-81
equ trap_RealTime							-82
equ trap_LAN_GetServerAddressString			-83
equ trap_LAN_GetServerInfo					-84
equ trap_LAN_MarkServerVisible 				-85
equ trap_LAN_UpdateVisiblePings				-86
equ trap_CIN_PlayCinematic					-87
equ trap_CIN_StopCinematic					-88
equ trap_CIN_RunCinematic 					-89
equ trap_CIN_DrawCinematic					-90
equ trap_CIN_SetExtents						-91
equ trap_R_RemapShader						-92
equ trap_LAN_ServerStatus					-93
equ trap_LAN_GetServerPing					-94
equ trap_LAN_ServerIsVisible				-95
equ trap_LAN_CompareServers					-96
equ	trap_Get_Radar							-97
equ trap_FS_Seek							-98
equ trap_SetPbClStatus						-99
equ trap_Con_Print							-100
equ trap_Con_Draw							-101
equ trap_Con_HandleMouse					-102
equ trap_Con_HandleKey						-103
equ trap_Con_Clear							-104
equ trap_Con_Resize							-105
equ trap_Con_GetText						-106
equ	memset									-107
equ	memcpy									-108
equ	strncpy									-109
equ	sin										-110
equ	cos										-111
equ	atan2									-112
equ	sqrt									-113
equ floor									-114
equ	ceil									-115
equ acos									-116
equ log										-117
equ pow										-118
equ atan									-119
equ fmod									-120
equ tan										-121

equ Q_rand									-501

equ trap_SQL_LoadDB							-601
equ trap_SQL_Exec							-602
equ trap_SQL_Prepare						-603
equ trap_SQL_Bind							-604
equ trap_SQL_BindText						-605
equ trap_SQL_BindInt						-606
equ trap_SQL_BindArgs						-607
equ trap_SQL_Step							-608
equ trap_SQL_ColumnCount					-609
equ trap_SQL_ColumnAsText					-610
equ trap_SQL_ColumnAsInt					-611
equ trap_SQL_ColumnName						-612
equ trap_SQL_Done							-613
equ trap_SQL_Select							-614
equ trap_SQL_Compile						-615
equ trap_SQL_Run							-616
equ trap_SQL_SelectFromServer				-617

