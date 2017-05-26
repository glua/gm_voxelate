#include "BaseVSShader.h"

#include "fxctmp9/voxels_vs20.inc"
#include "fxctmp9/voxels_ps20.inc"

BEGIN_VS_SHADER(Voxel_Shader, "Git gud.")

BEGIN_SHADER_PARAMS
END_SHADER_PARAMS

SHADER_INIT
{
	LoadTexture(BASETEXTURE, TEXTUREFLAGS_SRGB);
}

SHADER_FALLBACK
{
	return nullptr;
}

SHADER_DRAW
{
	SHADOW_STATE
	{

		pShaderShadow->VertexShaderVertexFormat(VERTEX_POSITION | VERTEX_NORMAL, 1, 0, 0);
		
		pShaderShadow->EnableTexture(SHADER_SAMPLER0, true);
		pShaderShadow->EnableSRGBRead(SHADER_SAMPLER0, true);

		pShaderShadow->EnableSRGBWrite(true);

		DECLARE_STATIC_VERTEX_SHADER(voxels_vs20);
		SET_STATIC_VERTEX_SHADER(voxels_vs20);

		DECLARE_STATIC_PIXEL_SHADER(voxels_ps20);
		SET_STATIC_PIXEL_SHADER(voxels_ps20);
	}
	
	DYNAMIC_STATE
	{
		BindTexture(SHADER_SAMPLER0, BASETEXTURE);

		pShaderAPI->SetVertexShaderStateAmbientLightCube();
		
		DECLARE_DYNAMIC_VERTEX_SHADER(voxels_vs20);
		SET_DYNAMIC_VERTEX_SHADER(voxels_vs20);

		DECLARE_DYNAMIC_PIXEL_SHADER(voxels_ps20);
		SET_DYNAMIC_PIXEL_SHADER(voxels_ps20);
	}

	Draw();
}

END_SHADER