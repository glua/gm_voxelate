//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
// An interface that should not ever be accessed directly from shaders
// but instead is visible only to shaderlib.
//===========================================================================//

#ifndef ISHADERSYSTEM_H
#define ISHADERSYSTEM_H

#ifdef _WIN32
#pragma once
#endif

#include "ishader.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
enum Sampler_t;
class ITexture;
class IShader;


//-----------------------------------------------------------------------------
// The Shader system interface version
//-----------------------------------------------------------------------------
#define SHADERSYSTEM_INTERFACE_VERSION		"ShaderSystem002"


//-----------------------------------------------------------------------------
// Modulation flags
//-----------------------------------------------------------------------------
enum
{
	SHADER_USING_COLOR_MODULATION		= 0x1,
	SHADER_USING_ALPHA_MODULATION		= 0x2,
	SHADER_USING_FLASHLIGHT				= 0x4,
	SHADER_USING_FIXED_FUNCTION_BAKED_LIGHTING		= 0x8,
	SHADER_USING_EDITOR					= 0x10,
};

typedef int ShaderAPITextureHandle_t;

//-----------------------------------------------------------------------------
// The shader system (a singleton)
//-----------------------------------------------------------------------------
abstract_class IShaderSystem
{
public:
	virtual ShaderAPITextureHandle_t GetShaderAPITextureBindHandle( ITexture *pTexture, int nFrameVar, int nTextureChannel = 0 ) =0;

	// Binds a texture
	virtual void BindTexture( Sampler_t sampler1, ITexture *pTexture, int nFrameVar = 0 ) = 0;
	virtual void BindTexture( Sampler_t sampler1, Sampler_t sampler2, ITexture *pTexture, int nFrameVar = 0 ) = 0;

	// Takes a snapshot
	virtual void TakeSnapshot( ) = 0;

	// Draws a snapshot
	virtual void DrawSnapshot( bool bMakeActualDrawCall = true ) = 0;

	// Are we using graphics?
	virtual bool IsUsingGraphics() const = 0;

	// Are we using graphics?
	virtual bool CanUseEditorMaterials() const = 0;
};


//-----------------------------------------------------------------------------
// The Shader plug-in DLL interface version
//-----------------------------------------------------------------------------
#define SHADER_DLL_INTERFACE_VERSION	"ShaderDLL004"


//-----------------------------------------------------------------------------
// The Shader interface versions
//-----------------------------------------------------------------------------
abstract_class IShaderDLLInternal
{
public:
	// Here's where the app systems get to learn about each other 
	virtual bool Connect( CreateInterfaceFn factory, bool bIsMaterialSystem ) = 0;
	virtual void Disconnect( bool bIsMaterialSystem ) = 0;

	// Returns the number of shaders defined in this DLL
	virtual int ShaderCount() const = 0;

	// Returns information about each shader defined in this DLL
	virtual IShader *GetShader( int nShader ) = 0;
};


//-----------------------------------------------------------------------------
// Singleton interface
//-----------------------------------------------------------------------------
IShaderDLLInternal *GetShaderDLLInternal();

enum
{
	MAX_RENDER_PASSES = 4
};

typedef short StateSnapshot_t;

struct RenderPassList_t
{
	int m_nPassCount;
	StateSnapshot_t	m_Snapshot[MAX_RENDER_PASSES];
	// per material shader-defined state
	CBasePerMaterialContextData *m_pContextData[MAX_RENDER_PASSES];
};

struct ShaderRenderState_t
{
	// These are the same, regardless of whether alpha or color mod is used
	int				m_Flags;	// Can't shrink this to a short
	VertexFormat_t	m_VertexFormat;
	VertexFormat_t	m_VertexUsage;
	MorphFormat_t	m_MorphFormat;

	// List of all snapshots
	RenderPassList_t *m_pSnapshots;


};

//-----------------------------------------------------------------------------
// The shader system (a singleton)
//-----------------------------------------------------------------------------
abstract_class IShaderSystemInternal : public IShaderInit, public IShaderSystem
{
public:
	// Initialization, shutdown
	virtual void		Init() = 0;
	virtual void		Shutdown() = 0;
	virtual void		ModInit() = 0;
	virtual void		ModShutdown() = 0;

	// Methods related to reading in shader DLLs
	virtual bool		LoadShaderDLL( const char *pFullPath ) = 0;
	virtual void		UnloadShaderDLL( const char *pFullPath ) = 0;

	// Find me a shader!
	virtual IShader*	FindShader( char const* pShaderName ) = 0;

	// returns strings associated with the shader state flags...
	virtual char const* ShaderStateString( int i ) const = 0;
	virtual int ShaderStateCount( ) const = 0;

	// Rendering related methods

	// Create debugging materials
	virtual void CreateDebugMaterials() = 0;

	// Cleans up the debugging materials
	virtual void CleanUpDebugMaterials() = 0;

	// Call the SHADER_PARAM_INIT block of the shaders
	virtual void InitShaderParameters( IShader *pShader, IMaterialVar **params, const char *pMaterialName ) = 0;

	// Call the SHADER_INIT block of the shaders
	virtual void InitShaderInstance( IShader *pShader, IMaterialVar **params, const char *pMaterialName, const char *pTextureGroupName ) = 0;

	// go through each param and make sure it is the right type, load textures, 
	// compute state snapshots and vertex types, etc.
	virtual bool InitRenderState( IShader *pShader, int numParams, IMaterialVar **params, ShaderRenderState_t* pRenderState, char const* pMaterialName ) = 0;

	// When you're done with the shader, be sure to call this to clean up
	virtual void CleanupRenderState( ShaderRenderState_t* pRenderState ) = 0;

	// Draws the shader
	virtual void DrawElements( IShader *pShader, IMaterialVar **params, ShaderRenderState_t* pShaderState, VertexCompressionType_t vertexCompression,
							   uint32 nMaterialVarTimeStamp ) = 0;

	// Used to iterate over all shaders for editing purposes
	virtual int	 ShaderCount() const = 0;
	virtual int  GetShaders( int nFirstShader, int nCount, IShader **ppShaderList ) const = 0;
};


#endif // ISHADERSYSTEM_H
