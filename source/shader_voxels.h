#include "shaders/BaseVSShader.h"
#include "vox_util.h"

BEGIN_VS_SHADER(Voxel_Shader, "Git gud.")

BEGIN_SHADER_PARAMS
END_SHADER_PARAMS

/*SHADER_INIT_PARAMS()
{
	vox_print("init params");
}*/
SHADER_INIT
{
	vox_print("init shader");
}

SHADER_FALLBACK
{
	vox_print("fall back");
	return nullptr;
}

SHADER_DRAW
{
	// 1 = compressed verts
	// 2 = water fog
	// 4 = skinning
	// 8 = lighting pv
	// 16 = light count

	SHADOW_STATE
	{
		SetInitialShadowState();
		pShaderShadow->SetVertexShader("black_vs20", 0);
		pShaderShadow->SetPixelShader("black_ps20", 0);
		
		//vox_print("draw shadow");
		//DECLARE_STATIC_VERTEX_SHADER(dingus);
		//SET_STATIC_VERTEX_SHADER()
	}
	
	DYNAMIC_STATE
	{
		//SET_DYNAMIC_PIXEL_SHADER
		pShaderAPI->SetDefaultState();
		pShaderAPI->SetVertexShaderIndex(0);
		pShaderAPI->SetPixelShaderIndex(0);
		//vox_print("draw dynamic");
	}

	//vox_print("pre");
	Draw();
	//vox_print("post");
}

END_SHADER