/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2007 HermitWorks Entertainment Corporation

This file is part of the Space Trader source code.

The Space Trader source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

The Space Trader source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with the Space Trader source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "ui_local.h"

// this file is only included when building a dll
// syscalls.asm is included instead when building a qvm
#ifdef Q3_VM
#error "Do not use in VM build"
#endif


static int (QDECL *syscall)( int arg, ... ) = (int (QDECL *)( int, ...))-1;

void QDECL dllEntry( int (QDECL *syscallptr)( int arg,... ) ) {
	syscall = syscallptr;
}

int PASSFLOAT( float x ) {
	fi_t t;
	t.f = x;
	return t.i;
}

void trap_Print( const char *string ) {
	syscall( UI_PRINT, string );
}

void trap_Error( int level, const char *string ) {
	syscall( UI_ERROR, level, string );
}

int trap_Milliseconds( void ) {
	return syscall( UI_MILLISECONDS ); 
}

void trap_Cvar_Register( vmCvar_t *cvar, const char *var_name, const char *value, int flags ) {
	syscall( UI_CVAR_REGISTER, cvar, var_name, value, flags );
}

void trap_Cvar_Update( vmCvar_t *cvar ) {
	syscall( UI_CVAR_UPDATE, cvar );
}

void trap_Cvar_Set( const char *var_name, const char *value ) {
	syscall( UI_CVAR_SET, var_name, value );
}

float trap_Cvar_VariableValue( const char *var_name ) {
	fi_t t;
	t.i = syscall( UI_CVAR_VARIABLEVALUE, var_name );
	return t.f;
}

int trap_Cvar_VariableInt( const char * var_name ) {
	return syscall( UI_CVAR_VARIABLEINT, var_name );
}

void trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize ) {
	syscall( UI_CVAR_VARIABLESTRINGBUFFER, var_name, buffer, bufsize );
}

void trap_Cvar_SetValue( const char *var_name, float value ) {
	syscall( UI_CVAR_SETVALUE, var_name, PASSFLOAT( value ) );
}

void trap_Cvar_Reset( const char *name ) {
	syscall( UI_CVAR_RESET, name ); 
}

void trap_Cvar_Create( const char *var_name, const char *var_value, int flags ) {
	syscall( UI_CVAR_CREATE, var_name, var_value, flags );
}

void trap_Cvar_InfoStringBuffer( int bit, char *buffer, int bufsize ) {
	syscall( UI_CVAR_INFOSTRINGBUFFER, bit, buffer, bufsize );
}

int trap_Argc( void ) {
	return syscall( UI_ARGC );
}

void trap_Argv( int n, char *buffer, int bufferLength ) {
	syscall( UI_ARGV, n, buffer, bufferLength );
}

int trap_ArgvI( int n ) {
	return syscall( UI_ARGVI, n );
}

void trap_Cmd_ExecuteText( int exec_when, const char *text ) {
	syscall( UI_CMD_EXECUTETEXT, exec_when, text );
}

int trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode ) {
	return syscall( UI_FS_FOPENFILE, qpath, f, mode );
}

void trap_FS_Read( void *buffer, int len, fileHandle_t f ) {
	syscall( UI_FS_READ, buffer, len, f );
}

void trap_FS_Write( const void *buffer, int len, fileHandle_t f ) {
	syscall( UI_FS_WRITE, buffer, len, f );
}

void trap_FS_FCloseFile( fileHandle_t f ) {
	syscall( UI_FS_FCLOSEFILE, f );
}

int trap_FS_GetFileList(  const char *path, const char *extension, char *listbuf, int bufsize ) {
	return syscall( UI_FS_GETFILELIST, path, extension, listbuf, bufsize );
}

int trap_FS_Seek( fileHandle_t f, long offset, int origin ) {
	return syscall( UI_FS_SEEK, f, offset, origin );
}

qhandle_t trap_R_RegisterModel( const char *name ) {
	return syscall( UI_R_REGISTERMODEL, name );
}

qhandle_t trap_R_RegisterSkin( const char *name ) {
	return syscall( UI_R_REGISTERSKIN, name );
}

qhandle_t trap_R_RegisterFont(const char *fontName) {
	return syscall( UI_R_REGISTERFONT, fontName );
}

void trap_R_GetFonts( char * buffer, int size ) {
	syscall( UI_R_GETFONTS, buffer, size );
}

qhandle_t trap_R_RegisterShader( const char *name ) {
	return syscall( UI_R_REGISTERSHADER, name );
}

qhandle_t trap_R_RegisterShaderNoMip( const char *name ) {
	return syscall( UI_R_REGISTERSHADERNOMIP, name );
}

void trap_R_ClearScene( void ) {
	syscall( UI_R_CLEARSCENE );
}

qhandle_t trap_R_BuildPose( qhandle_t hModel, const animGroupFrame_t *groupFrames, uint numGroupFrames, boneOffset_t *boneOffsets, uint numBoneOffsets )
{
	return syscall( UI_R_BUILDPOSE, hModel, groupFrames, numGroupFrames, boneOffsets, numBoneOffsets );
}
qhandle_t trap_R_BuildPose2( qhandle_t hModel,	const animGroupFrame_t *groupFrames0, const boneOffset_t *boneOffsets0,
											const animGroupFrame_t *groupFrames1, const boneOffset_t *boneOffsets1,
											const animGroupTransition_t *frameLerps )
{
	return syscall( UI_R_BUILDPOSE2, hModel, groupFrames0, boneOffsets0, groupFrames1, boneOffsets1, frameLerps );
}

qhandle_t trap_R_BuildPose3( qhandle_t hModel, const animGroupFrame_t *groupFrames0, const animGroupFrame_t *groupFrames1, const animGroupTransition_t *frameLerps )
{
	return syscall( UI_R_BUILDPOSE3, hModel, groupFrames0, groupFrames1, frameLerps );
}

qhandle_t trap_R_ShapeCreate( const curve_t *curves, const shapeGen_t *genTypes, int numShapes )
{
	return syscall( UI_R_SHAPECREATE, curves, genTypes, numShapes );
}

void trap_R_ShapeDraw( qhandle_t shape, qhandle_t shader, float * m )
{
	syscall( UI_R_SHAPEDRAW, shape, shader, m );
}

void trap_R_ShapeDrawMulti( void **shapes, unsigned int numShapes, qhandle_t shader )
{
	syscall( UI_R_SHAPEDRAWMULTI, shapes, numShapes, shader );
}

void trap_R_AddRefEntityToScene( const refEntity_t *re ) {
	syscall( UI_R_ADDREFENTITYTOSCENE, re );
}

void trap_R_AddPolyToScene( qhandle_t hShader , int numVerts, const polyVert_t *verts ) {
	syscall( UI_R_ADDPOLYTOSCENE, hShader, numVerts, verts );
}

void trap_R_AddLightToScene( const vec3_t org, float intensity, float r, float g, float b ) {
	syscall( UI_R_ADDLIGHTTOSCENE, org, PASSFLOAT(intensity), PASSFLOAT(r), PASSFLOAT(g), PASSFLOAT(b) );
}

void trap_R_RenderScene( const refdef_t *fd ) {
	syscall( UI_R_RENDERSCENE, fd );
}

void trap_R_SetColor( const float *rgba ) {
	syscall( UI_R_SETCOLOR, rgba );
}

void trap_R_DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader ) {
	syscall( UI_R_DRAWSTRETCHPIC, PASSFLOAT(x), PASSFLOAT(y), PASSFLOAT(w), PASSFLOAT(h), PASSFLOAT(s1), PASSFLOAT(t1), PASSFLOAT(s2), PASSFLOAT(t2), hShader );
}

void trap_R_DrawSprite( float x, float y, float w, float h, float mat[2][3], vec4_t uv, qhandle_t hShader ) {
	syscall( UI_R_DRAWSPRITE, PASSFLOAT(x), PASSFLOAT(y), PASSFLOAT(w), PASSFLOAT(h), mat, uv, hShader );
}

void trap_R_DrawText(	Rectangle*			rect,		// render within this rectangle
						float				scale,		// scale the text
						const float*		rgba,
						const char*			text,
						int					limit,		// don't draw more than this many characters
						int					align,
						int					style,		// shadow etc
						int					cursor,		// >= 0 draws a flashing cusor at text location
						qhandle_t			font,
						const rectDef_t*	textRect	// return the rectangle that fits tight around the text
				)
{
	syscall( UI_R_RENDERTEXT, rect, PASSFLOAT(scale), rgba, text, limit, align, style, cursor, font, textRect );
}

int trap_R_FlowText( fontInfo_t * font, float fontScale, const float startColor[4], const float baseColor[4], const char * text, int limit, int align, float width, lineInfo_t * lines, int max ) {
	return syscall( UI_R_FLOWTEXT, font, PASSFLOAT(fontScale), startColor, baseColor, text, limit, align, PASSFLOAT(width), lines, max );
}

void trap_R_GetFont( qhandle_t font, float scale, fontInfo_t * buffer )
{
	syscall( UI_R_GETFONT, font, PASSFLOAT(scale), buffer );
}

void trap_R_RenderRoundRect( float x, float y, float w, float h, const float * rgba, qhandle_t shader )
{
	syscall( UI_R_ROUNDRECT, PASSFLOAT(x), PASSFLOAT(y), PASSFLOAT(w), PASSFLOAT(h), rgba, shader );
}

void	trap_R_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs ) {
	syscall( UI_R_MODELBOUNDS, model, mins, maxs );
}

void trap_UpdateScreen( void ) {
	syscall( UI_UPDATESCREEN );
}

int trap_CM_LerpTag( affine_t *tag, clipHandle_t mod, int startFrame, int endFrame, float frac, const char *tagName ) {
	return syscall( UI_CM_LERPTAG, tag, mod, startFrame, endFrame, PASSFLOAT(frac), tagName );
}

void trap_S_StartLocalSound( sfxHandle_t sfx, int channelNum ) {
	syscall( UI_S_STARTLOCALSOUND, sfx, channelNum );
}

sfxHandle_t	trap_S_RegisterSound( const char *sample, qboolean compressed ) {
	return syscall( UI_S_REGISTERSOUND, sample, compressed );
}

void trap_Key_KeynumToStringBuf( int keynum, char *buf, int buflen ) {
	syscall( UI_KEY_KEYNUMTOSTRINGBUF, keynum, buf, buflen );
}

void trap_Key_GetBindingBuf( int keynum, char *buf, int buflen ) {
	syscall( UI_KEY_GETBINDINGBUF, keynum, buf, buflen );
}

void trap_Key_SetBinding( int keynum, const char *binding ) {
	syscall( UI_KEY_SETBINDING, keynum, binding );
}

qboolean trap_Key_IsDown( int keynum ) {
	return syscall( UI_KEY_ISDOWN, keynum );
}

qboolean trap_Key_GetOverstrikeMode( void ) {
	return syscall( UI_KEY_GETOVERSTRIKEMODE );
}

void trap_Key_SetOverstrikeMode( qboolean state ) {
	syscall( UI_KEY_SETOVERSTRIKEMODE, state );
}

void trap_Key_ClearStates( void ) {
	syscall( UI_KEY_CLEARSTATES );
}

int trap_Key_GetCatcher( void ) {
	return syscall( UI_KEY_GETCATCHER );
}

void trap_Key_SetCatcher( int catcher ) {
	syscall( UI_KEY_SETCATCHER, catcher );
}

int trap_Key_GetKey( const char *binding ) {
	return syscall( UI_KEY_GETKEY, binding );
}

void trap_GetClipboardData( char *buf, int bufsize ) {
	syscall( UI_GETCLIPBOARDDATA, buf, bufsize );
}

void trap_GetClientState( uiClientState_t *state ) {
	syscall( UI_GETCLIENTSTATE, state );
}

int trap_GetClientNum( void ) {
	return syscall( UI_GETCLIENTNUM );
}

void trap_GetGlconfig( vidConfig_t *glconfig ) {
	syscall( UI_GETGLCONFIG, glconfig );
}

int trap_GetConfigString( int index, char* buff, int buffsize ) {
	return syscall( UI_GETCONFIGSTRING, index, buff, buffsize );
}

int trap_UpdateGameState( globalState_t * gs )
{
	return syscall( UI_UPDATEGAMESTATE, gs );
}

qboolean trap_LAN_UpdateVisiblePings( int source ) {
	return syscall( UI_LAN_UPDATEVISIBLEPINGS, source );
}

int trap_MemoryRemaining( void ) {
	return syscall( UI_MEMORY_REMAINING );
}

int trap_PC_AddGlobalDefine( char *define ) {
	return syscall( UI_PC_ADD_GLOBAL_DEFINE, define );
}

int trap_PC_LoadSource( const char *filename ) {
	return syscall( UI_PC_LOAD_SOURCE, filename );
}

int trap_PC_FreeSource( int handle ) {
	return syscall( UI_PC_FREE_SOURCE, handle );
}

int trap_PC_ReadToken( int handle, pc_token_t *pc_token ) {
	return syscall( UI_PC_READ_TOKEN, handle, pc_token );
}

int trap_PC_SourceFileAndLine( int handle, char *filename, int *line ) {
	return syscall( UI_PC_SOURCE_FILE_AND_LINE, handle, filename, line );
}

void trap_S_StopBackgroundTrack( void ) {
	syscall( UI_S_STOPBACKGROUNDTRACK );
}

void trap_S_StartBackgroundTrack( const char *intro, const char *loop) {
	syscall( UI_S_STARTBACKGROUNDTRACK, intro, loop );
}

int trap_RealTime(qtime_t *qtime) {
	return syscall( UI_REAL_TIME, qtime );
}

// this returns a handle.  arg0 is the name in the format "idlogo.roq", set arg1 to NULL, alteredstates to qfalse (do not alter gamestate)
int trap_CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits) {
  return syscall(UI_CIN_PLAYCINEMATIC, arg0, xpos, ypos, width, height, bits);
}
 
// stops playing the cinematic and ends it.  should always return FMV_EOF
// cinematics must be stopped in reverse order of when they are started
e_status trap_CIN_StopCinematic(int handle) {
  return syscall(UI_CIN_STOPCINEMATIC, handle);
}


// will run a frame of the cinematic but will not draw it.  Will return FMV_EOF if the end of the cinematic has been reached.
e_status trap_CIN_RunCinematic (int handle) {
  return syscall(UI_CIN_RUNCINEMATIC, handle);
}
 

// draws the current frame
void trap_CIN_DrawCinematic (int handle) {
  syscall(UI_CIN_DRAWCINEMATIC, handle);
}
 

// allows you to resize the animation dynamically
void trap_CIN_SetExtents (int handle, int x, int y, int w, int h) {
  syscall(UI_CIN_SETEXTENTS, handle, x, y, w, h);
}


void trap_R_RemapShader( const char *oldShader, const char *newShader )
{
	syscall( UI_R_REMAP_SHADER, oldShader, newShader );
}

void trap_SetPbClStatus( int status ) {
	syscall( UI_SET_PBCLSTATUS, status );
}

int trap_Con_Print( int c, const char * text ) {
	return syscall( UI_CON_PRINT, c, text );
}
void trap_Con_Draw( int c, float frac ) {
	syscall( UI_CON_DRAW, c, PASSFLOAT(frac) );
}
void trap_Con_HandleMouse( int c, float x, float y ) {
	syscall( UI_CON_HANDLEMOUSE, c, PASSFLOAT(x), PASSFLOAT(y) );
}
void trap_Con_HandleKey( int c, int key ) {
	syscall( UI_CON_HANDLEKEY, c, key );
}
void trap_Con_Clear( int c ) {
	syscall( UI_CON_CLEAR, c );
}
void trap_Con_Resize( int c, int display, float x, float y, float w, float h ) {
	syscall( UI_CON_RESIZE, c, display, PASSFLOAT(x), PASSFLOAT(y), PASSFLOAT(w), PASSFLOAT(h) );
}

void trap_Con_GetText( char *buf, int buf_size, int c )
{
	syscall( UI_CON_GETTEXT, buf, buf_size, c );
}

int trap_Get_Radar( radarInfo_t* radar, int maxEnts, const radarFilter_t *filters, int numFilters, float range )
{
	return syscall( UI_GET_RADAR, radar, maxEnts, filters, numFilters, PASSFLOAT(range) );
}

int Q_rand() {
	return syscall( UI_Q_rand );
}


//
//	SQL interface
//
void trap_SQL_LoadDB( const char * filename, const char * filter ) {
	syscall( UI_SQL_LOADDB, filename, filter );
}

void trap_SQL_Exec( const char* statement ) {
	syscall( UI_SQL_EXEC, statement );
}

qhandle_t trap_SQL_Prepare( const char * src ) {
	return syscall( UI_SQL_PREPARE, src );
}

void trap_SQL_BindText( int i, const char* text ) {
	syscall( UI_SQL_BINDTEXT, i, text );
}

void trap_SQL_BindInt( int i, int v ) {
	syscall( UI_SQL_BINDINT, i, v );
}

void trap_SQL_BindArgs() {
	syscall( UI_SQL_BINDARGS );
}

qboolean trap_SQL_Step() {
	return syscall( UI_SQL_STEP );
}

int trap_SQL_ColumnCount() {
	return syscall( UI_SQL_COLUMNCOUNT );
}

void trap_SQL_ColumnAsText( char * buffer, int size, int i ) {
	syscall( UI_SQL_COLUMNASTEXT, buffer, size, i );
}

int trap_SQL_ColumnAsInt( int i ) {
	return syscall( UI_SQL_COLUMNASINT, i );
}

void trap_SQL_ColumnName( char * buffer, int size, int i ) {
	syscall( UI_SQL_COLUMNNAME, buffer, size, i );
}

int trap_SQL_Done() {
	return syscall( UI_SQL_DONE );
}

int trap_SQL_Select( const char * src, char * buffer, int size ) {
	return syscall( UI_SQL_SELECT, src, buffer, size );
}

int trap_SQL_SelectFromServer( const char * src, char * buffer, int size ) {
	return syscall( UI_SQL_SV_SELECT, src, buffer, size );
}

int trap_SQL_Compile( const char * src ) {
	return syscall( UI_SQL_COMPILE, src );
}

void trap_SQL_Run( char * buffer, int size, int index, int p1, int p2, int p3 ) {
	syscall( UI_SQL_RUN, buffer, size, index, p1,p2,p3 );
}
