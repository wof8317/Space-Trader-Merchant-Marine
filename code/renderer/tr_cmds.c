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

#include "tr_local.h"

volatile renderCommandList_t	*renderCommandList;

volatile qboolean	renderThreadActive;


/*
=====================
R_PerformanceCounters
=====================
*/
void R_PerformanceCounters( void ) {
	if ( !r_speeds->integer ) {
		// clear the counters even if we aren't printing
		Com_Memset( &tr.pc, 0, sizeof( tr.pc ) );
		Com_Memset( &backEnd.pc, 0, sizeof( backEnd.pc ) );
		return;
	}

	if (r_speeds->integer == 1) {
		ri.Printf (PRINT_ALL, "%i/%i shaders/surfs %i leafs %i verts %i/%i tris %.2f mtex %.2f dc\n",
			backEnd.pc.c_shaders, backEnd.pc.c_surfaces, tr.pc.c_leafs, backEnd.pc.c_vertexes, 
			backEnd.pc.c_indexes/3, backEnd.pc.c_totalIndexes/3, 
			R_SumOfUsedImages()/(1000000.0f), backEnd.pc.c_overDraw / (float)(glConfig.vidWidth * glConfig.vidHeight) ); 
	} else if (r_speeds->integer == 2) {
		ri.Printf (PRINT_ALL, "(patch) %i sin %i sclip  %i sout %i bin %i bclip %i bout\n",
			tr.pc.c_sphere_cull_patch_in, tr.pc.c_sphere_cull_patch_clip, tr.pc.c_sphere_cull_patch_out, 
			tr.pc.c_box_cull_patch_in, tr.pc.c_box_cull_patch_clip, tr.pc.c_box_cull_patch_out );
		ri.Printf (PRINT_ALL, "(md3) %i sin %i sclip  %i sout %i bin %i bclip %i bout\n",
			tr.pc.c_sphere_cull_md3_in, tr.pc.c_sphere_cull_md3_clip, tr.pc.c_sphere_cull_md3_out, 
			tr.pc.c_box_cull_md3_in, tr.pc.c_box_cull_md3_clip, tr.pc.c_box_cull_md3_out );
	} else if (r_speeds->integer == 3) {
		ri.Printf (PRINT_ALL, "viewcluster: %i\n", tr.viewCluster );
	} else if (r_speeds->integer == 4) {
		if ( backEnd.pc.c_dlightVertexes ) {
			ri.Printf (PRINT_ALL, "dlight srf:%i  culled:%i  verts:%i  tris:%i\n", 
				tr.pc.c_dlightSurfaces, tr.pc.c_dlightSurfacesCulled,
				backEnd.pc.c_dlightVertexes, backEnd.pc.c_dlightIndexes / 3 );
		}
	} 
	else if (r_speeds->integer == 5 )
	{
		ri.Printf( PRINT_ALL, "zFar: %.0f\n", tr.viewParms.zFar );
	}
	else if (r_speeds->integer == 6 )
	{
		ri.Printf( PRINT_ALL, "flare adds:%i tests:%i renders:%i\n", 
			backEnd.pc.c_flareAdds, backEnd.pc.c_flareTests, backEnd.pc.c_flareRenders );
	}
	else if( r_speeds->integer == 7 )
	{
		ri.Printf( PRINT_ALL, "%i draw calls\n", backEnd.pc.c_drawCalls );
	}

	Com_Memset( &tr.pc, 0, sizeof( tr.pc ) );
	Com_Memset( &backEnd.pc, 0, sizeof( backEnd.pc ) );
}


/*
====================
R_InitCommandBuffers
====================
*/
void R_InitCommandBuffers( void ) {
	glConfig.smpActive = qfalse;
	if ( r_smp->integer ) {
		ri.Printf( PRINT_ALL, "Trying SMP acceleration...\n" );
		if ( GLimp_SpawnRenderThread( RB_RenderThread ) ) {
			ri.Printf( PRINT_ALL, "...succeeded.\n" );
			glConfig.smpActive = qtrue;
		} else {
			ri.Printf( PRINT_ALL, "...failed.\n" );
		}
	}
}

/*
====================
R_ShutdownCommandBuffers
====================
*/
void R_ShutdownCommandBuffers( void ) {
	// kill the rendering thread
	if ( glConfig.smpActive ) {
		GLimp_WakeRenderer( NULL );
		glConfig.smpActive = qfalse;
	}
}

/*
====================
R_IssueRenderCommands
====================
*/
int	c_blockedOnRender;
int	c_blockedOnMain;

void R_IssueRenderCommands( qboolean runPerformanceCounters ) {
	renderCommandList_t	*cmdList;

	cmdList = &backEndData[tr.smpFrame]->commands;
	ASSERT( cmdList ); // bk001205
	
	//add the end-of-list marker
	if( cmdList->cmdsUsed < MAX_RENDER_COMMANDS )
		cmdList->cmds[cmdList->cmdsUsed].func = NULL;

	//clear it out in case this is a sync and not a buffer flip
	cmdList->cmdsUsed = 0;
	cmdList->dataUsed = 0;

	if ( glConfig.smpActive ) {
		// if the render thread is not idle, wait for it
		if ( renderThreadActive ) {
			c_blockedOnRender++;
			if ( r_showSmp->integer ) {
				ri.Printf( PRINT_ALL, "R" );
			}
		} else {
			c_blockedOnMain++;
			if ( r_showSmp->integer ) {
				ri.Printf( PRINT_ALL, "." );
			}
		}

		// sleep until the renderer has completed
		GLimp_FrontEndSleep();
	}

	// at this point, the back end thread is idle, so it is ok
	// to look at it's performance counters
	if ( runPerformanceCounters ) {
		R_PerformanceCounters();
	}

	// actually start the commands going
	if ( !r_skipBackEnd->integer ) {
		// let it start on the new batch
		if ( !glConfig.smpActive ) {
			RB_ExecuteRenderCommands( cmdList );
		} else {
			GLimp_WakeRenderer( cmdList );
		}
	}
}


/*
====================
R_SyncRenderThread

Issue any pending commands and wait for them to complete.
After exiting, the render thread will have completed its work
and will remain idle and the main thread is free to issue
OpenGL calls until R_IssueRenderCommands is called.
====================
*/
void R_SyncRenderThread( void ) {
	if ( !tr.registered ) {
		return;
	}
	R_IssueRenderCommands( qfalse );

	if ( !glConfig.smpActive ) {
		return;
	}
	GLimp_FrontEndSleep();
}

/*
============
R_GetCommandBuffer

make sure there is enough command space, waiting on the
render thread if needed.
============
*/
void* R_GetCommandBuffer( renderCommandFunc_t cmd, uint bytes )
{
	renderCommandList_t	*cmdList;
	void *ret;

	cmdList = &backEndData[tr.smpFrame]->commands;

	bytes = (uint)Align( bytes, 4 );

	if( cmdList->cmdsUsed == MAX_RENDER_COMMANDS )
		return NULL;

	if( cmdList->dataUsed + bytes >	MAX_RENDER_COMMAND_MEM )
		return NULL;

	ret = cmdList->data + cmdList->dataUsed;

	cmdList->cmds[cmdList->cmdsUsed].func = cmd;
	cmdList->cmds[cmdList->cmdsUsed].data = ret;

	cmdList->cmdsUsed++;
	cmdList->dataUsed += bytes;

	return ret;
}

void* R_GetCommandMemory( uint bytes )
{
	renderCommandList_t	*cmdList;
	void *ret;

	cmdList = &backEndData[tr.smpFrame]->commands;

	bytes = (uint)Align( bytes, 4 );

	if( cmdList->dataUsed + bytes >	MAX_RENDER_COMMAND_MEM )
		return NULL;

	ret = cmdList->data + cmdList->dataUsed;

	cmdList->dataUsed += bytes;

	return ret;
}

/*
====================
RE_BeginFrame

If running in stereo, RE_BeginFrame will be called twice
for each RE_EndFrame
====================
*/
void Q_EXTERNAL_CALL RE_BeginFrame( stereoFrame_t stereoFrame ) {
	drawBufferCommand_t	*cmd;

	if ( !tr.registered ) {
		return;
	}
	glState.finishCalled = qfalse;

	tr.frameCount++;
	tr.frameSceneNum = 0;

	//
	// do overdraw measurement
	//
	if ( r_measureOverdraw->integer )
	{
		if ( glConfig.stencilBits < 4 )
		{
			ri.Printf( PRINT_ALL, "Warning: not enough stencil bits to measure overdraw: %d\n", glConfig.stencilBits );
			ri.Cvar_Set( "r_measureOverdraw", "0" );
			r_measureOverdraw->modified = qfalse;
		}
		else
		{
			R_SyncRenderThread();
			glEnable( GL_STENCIL_TEST );
			glStencilMask( ~0U );
			glClearStencil( 0U );
			glStencilFunc( GL_ALWAYS, 0U, ~0U );
			glStencilOp( GL_KEEP, GL_INCR, GL_INCR );
		}
		r_measureOverdraw->modified = qfalse;
	}
	else
	{
		// this is only reached if it was on and is now off
		if ( r_measureOverdraw->modified ) {
			R_SyncRenderThread();
			glDisable( GL_STENCIL_TEST );
		}
		r_measureOverdraw->modified = qfalse;
	}

	//
	// texturemode stuff
	//
	if ( r_textureMode->modified ) {
		R_SyncRenderThread();
		R_StateSetTextureModeCvar( r_textureMode->string );
		r_textureMode->modified = qfalse;
	}

	if( r_textureAniso->modified )
	{
		R_SyncRenderThread();
		R_StateSetTextureAnisotropyCvar( r_textureAniso->integer );
		r_textureAniso->modified = qfalse;
	}

	if( r_textureLod->modified )
	{
		R_SyncRenderThread();
		R_StateSetTextureMinLodCvar( r_textureLod->integer );
		r_textureLod->modified = qfalse;
	}

	//
	// gamma stuff
	//
	if( r_gamma->modified )
	{
		r_gamma->modified = qfalse;

		R_SyncRenderThread();
		R_SetColorMappings();
	}

	/*
	if( r_depthOfField->modified )
	{
		if( r_depthOfField->integer && !R_DofIsEnabled() )
			ri.Printf( PRINT_ALL, "Depth of field will not be enabled until a graphics restart.\n" );

		r_depthOfField->modified = qfalse;
	}
	*/

	if( r_bloom->modified )
	{
		if( r_bloom->integer > 0 && !R_BloomIsEnabled() )
			ri.Printf( PRINT_ALL, "Bloom will not be enabled until a graphics restart.\n" );

		r_bloom->modified = qfalse;
	}

    // check for errors
    if ( !r_ignoreGLErrors->integer ) {
        int	err;

		R_SyncRenderThread();
        if ( ( err = glGetError() ) != GL_NO_ERROR ) {
            ri.Error( ERR_FATAL, "RE_BeginFrame() - glGetError() failed (0x%x)!\n", err );
        }
    }

	//
	// draw buffer stuff
	//
	cmd = R_GetCommandBuffer( RB_DrawBuffer, sizeof( *cmd ) );
	if ( !cmd )
		return;
	
	if ( glConfig.stereoEnabled ) {
		if ( stereoFrame == STEREO_LEFT ) {
			cmd->buffer = (int)GL_BACK_LEFT;
		} else if ( stereoFrame == STEREO_RIGHT ) {
			cmd->buffer = (int)GL_BACK_RIGHT;
		} else {
			ri.Error( ERR_FATAL, "RE_BeginFrame: Stereo is enabled, but stereoFrame was %i", stereoFrame );
		}
	} else {
		if ( stereoFrame != STEREO_CENTER ) {
			ri.Error( ERR_FATAL, "RE_BeginFrame: Stereo is disabled, but stereoFrame was %i", stereoFrame );
		}
		if ( !Q_stricmp( r_drawBuffer->string, "GL_FRONT" ) ) {
			cmd->buffer = (int)GL_FRONT;
		} else {
			cmd->buffer = (int)GL_BACK;
		}
	}
}

/*
=============
RE_EndFrame

Returns the number of msec spent in the back end
=============
*/
void Q_EXTERNAL_CALL RE_EndFrame( int *frontEndMsec, int *backEndMsec )
{
	if ( !tr.registered ) {
		return;
	}
	
	R_GetCommandBuffer( RB_SwapBuffers, 0 ); //a NULL command is a special flag to trigger the end of frame
	
	R_IssueRenderCommands( qtrue );

	// use the other buffers next frame, because another CPU
	// may still be rendering into the current ones
	R_ToggleSmpFrame();

	if ( frontEndMsec ) {
		*frontEndMsec = tr.frontEndMsec;
	}
	tr.frontEndMsec = 0;
	if ( backEndMsec ) {
		*backEndMsec = backEnd.pc.msec;
	}
	backEnd.pc.msec = 0;
}

void Q_EXTERNAL_CALL RE_MenuSurfBegin( int surfNum )
{
}
void Q_EXTERNAL_CALL RE_MenuSurfEnd( void )
{
}
