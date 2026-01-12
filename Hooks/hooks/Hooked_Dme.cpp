#include "..\hooks.hpp"
#include "..\..\features\misc\fakelag.h"
#include "..\..\features\lagcompensation\local_animations.h"
#include "..\..\features\visuals\playeresp.h"

IMaterial* CreateMaterial(bool lit, const std::string& material_data)
{
	static auto created = 0;
	std::string type = lit ? crypt_str("VertexLitGeneric") : crypt_str("UnlitGeneric");

	auto matname = crypt_str("n0n4m3_") + std::to_string(created);
	++created;

	auto keyValues = new KeyValues(matname.c_str());
	static auto key_values_address = util::FindSignature(crypt_str("client.dll"), crypt_str("55 8B EC 51 33 C0 C7 45"));

	using KeyValuesFn = void(__thiscall*)(void*, const char*, int, int);
	reinterpret_cast <KeyValuesFn> (key_values_address)(keyValues, type.c_str(), 0, 0);

	static auto load_from_buffer_address = util::FindSignature(crypt_str("client.dll"), crypt_str("55 8B EC 83 E4 F8 83 EC 34 53 8B 5D 0C 89"));
	using LoadFromBufferFn = void(__thiscall*)(void*, const char*, const char*, void*, const char*, void*);

	reinterpret_cast <LoadFromBufferFn> (load_from_buffer_address)(keyValues, matname.c_str(), material_data.c_str(), nullptr, nullptr, nullptr);

	auto material = m_materialsystem()->CreateMaterial(matname.c_str(), keyValues);
	material->IncrementReferenceCount();

	return material;
}

using DrawModelExecute_t = void(__thiscall*)(IVModelRender*, IMatRenderContext*, const DrawModelState_t&, const ModelRenderInfo_t&, matrix3x4_t*);

void __stdcall hooks::hooked_dme(IMatRenderContext* ctx, const DrawModelState_t& state, const ModelRenderInfo_t& info, matrix3x4_t* bone_to_world)
{
	static auto original_fn = modelrender_hook->get_func_address <DrawModelExecute_t> (21);
	g_ctx.local((player_t*)m_entitylist()->GetClientEntity(m_engine()->GetLocalPlayer()), true);

	if (!cfg.player.enable)
		return original_fn(m_modelrender(), ctx, state, info, bone_to_world);

	if (!info.pModel)
		return original_fn(m_modelrender(), ctx, state, info, bone_to_world);

	if (!info.pRenderable)
		return original_fn(m_modelrender(), ctx, state, info, bone_to_world);

	auto model_entity = static_cast<player_t *>(m_entitylist()->GetClientEntity(info.entity_index));
	auto name = m_modelinfo()->GetModelName(info.pModel);

	auto is_player = strstr(name, "models/player") && model_entity->is_alive() && (cfg.player.type[ENEMY].chams[PLAYER_CHAMS_VISIBLE] || cfg.player.type[TEAM].chams[PLAYER_CHAMS_VISIBLE] || cfg.player.type[LOCAL].chams[PLAYER_CHAMS_VISIBLE] || cfg.player.fake_chams_enable || cfg.player.backtrack_chams);
	auto is_weapon = strstr(name, "weapons/v_") && !strstr(name, "arms") && cfg.esp.weapon_chams;
	//auto is_arms = strstr(name, "arms") && cfg.esp.arms_chams;
	//auto is_sleeve = strstr(name, "sleeve") && cfg.esp.arms_chams;
	auto weapon_on_back = strstr(name, "_dropped.mdl") && strstr(name, "models/weapons/w") && !strstr(name, "arms") && !strstr(name, "ied_dropped") && cfg.esp.attachment_chams;
	auto weapon_enemy_hands = strstr(name, "models/weapons/w") && !strstr(name, "arms") && !strstr(name, "ied_dropped") && cfg.esp.attachment_chams;
	auto defuse_kit = strstr(name, "defuser") && !strstr(name, "arms") && !strstr(name, "ied_dropped") && cfg.esp.attachment_chams;

	if (m_modelrender()->IsForcedMaterialOverride() && !is_weapon && !weapon_on_back && !weapon_enemy_hands && !defuse_kit)
		return original_fn(m_modelrender(), ctx, state, info, bone_to_world);

	m_renderview()->SetColorModulation(1.0f, 1.0f, 1.0f); 

	if (!is_player && !is_weapon && !weapon_on_back && !weapon_enemy_hands && !defuse_kit)
		return original_fn(m_modelrender(), ctx, state, info, bone_to_world);

	static IMaterial* materials[] =
	{
		CreateMaterial(true, crypt_str(R"#("VertexLitGeneric"
        {
            "$basetexture"    "vgui/white"
            "$envmap"       "env_cubemap"
            "$model"        "1"
            "$flat"            "0"
            "$nocull"        "1"
            "$halflambert"    "1"
            "$nofog"        "1"
            "$ignorez"        "0"
            "$znearer"        "0"
            "$wireframe"    "0"
        }
        )#")),
		CreateMaterial(false, crypt_str(R"#("UnlitGeneric"
            {
                "$basetexture"                "vgui/white"
                "$ignorez"                    "0"
                "$envmap"                    " "
                "$nofog"                    "1"
                "$model"                    "1"
                "$nocull"                    "0"
                "$selfillum"                "1"
                "$halflambert"                "1"
                "$znearer"                    "0"
                "$flat"                        "1"
                "$wireframe"                "0"
            }
        )#")),
		m_materialsystem()->FindMaterial(crypt_str("models/inventory_items/dogtags/dogtags_outline"), nullptr), //-V807
		m_materialsystem()->FindMaterial(crypt_str("models/inventory_items/cologne_prediction/cologne_prediction_glass"), nullptr),
				CreateMaterial(true, crypt_str(R"#("VertexLitGeneric" {
            "$basetexture"                    "vgui/white"
            "$envmap"                        "env_cubemap"
            "$envmaptint"                   "[.10 .10 .10]"
            "$pearlescent"                    "0"
            "$phong"                        "1"
            "$phongexponent"                "10"
            "$phongboost"                    "1.0"
            "$rimlight"                        "1"
            "$rimlightexponent"                "1"
            "$rimlightboost"                "1"
            "$model"                        "1"
            "$nocull"                        "0"
            "$halflambert"                    "1"
            "$lightwarptexture"             "metalic"
        }"
            {
                "$additive" "0.5"
                "$envmap" "models/effects/cube_white"
                "$ignorez"                    "1"
                "$envmaptint" "[1 1 1]"
                "$envmapfresnel" "1"
                "$envmapfresnelminmaxexp" "[0 1 2]"
                "$alpha" "1"
                "$wireframe"                "0"
            }
        )#")),
		CreateMaterial(true, crypt_str(R"#("VertexLitGeneric"
            {
                "$additive" "0.5"
                "$envmap" "models/effects/cube_white"
                "$ignorez"                    "1"
                "$envmaptint" "[1 1 1]"
                "$envmapfresnel" "1"
                "$envmapfresnelminmaxexp" "[0 1 2]"
                "$alpha" "1"
                "$wireframe"                "0"
            }
        )#")),
		CreateMaterial(true, crypt_str(R"#("VertexLitGeneric"
            {
                "$additive" "1"
                "$envmap" "models/effects/cube_white"
                "$envmaptint" "[1 1 1]"
                "$envmapfresnel" "1"
                "$envmapfresnelminmaxexp" "[0 1 2]"
                "$alpha" "0.8"
                "$wireframe"                "1"
            }
        )#")),
		CreateMaterial(true, crypt_str(R"#("VertexLitGeneric"
            {
            "$basetexture" "models/effects/cube_white"
            "$additive"                    "1"
            "$envmap"                    "models/effects/cube_white"
            "$envmaptint"                "[1.0 1.0. 1.0]"
            "$envmapfresnel"            "1.0"
            "$envmapfresnelminmaxexp"    "[0.0 1.0 2.0]"
            "$alpha"                    "0.99"
        }
        )#")),
		m_materialsystem()->FindMaterial(crypt_str("models/inventory_items/trophy_majors/velvet"), nullptr),
	};

	auto called_original = false;

	if (is_player)
	{
		auto type = ENEMY;

		if (model_entity == g_ctx.local())
			type = LOCAL;
		else if (model_entity->m_iTeamNum() == g_ctx.local()->m_iTeamNum())
			type = TEAM;

		if (type == ENEMY)
		{
			auto alpha_modifier = playeresp::get().esp_alpha_fade[model_entity->EntIndex()];

			auto material = materials[cfg.player.type[ENEMY].chams_type];
			auto double_material = materials[6];

			if (material && double_material && !material->IsErrorMaterial() && !double_material->IsErrorMaterial())
			{
				if (cfg.player.type[ENEMY].chams[PLAYER_CHAMS_VISIBLE] && cfg.player.type[ENEMY].chams[PLAYER_CHAMS_INVISIBLE])
				{
					auto alpha = (float)cfg.player.backtrack_chams_color.a() / 255.0f;;

					if (cfg.player.backtrack_chams)
					{
						auto backtrack_material = materials[cfg.player.backtrack_chams_material];

						if (backtrack_material && !backtrack_material->IsErrorMaterial())
						{
							matrix3x4_t matrix[MAXSTUDIOBONES];

							if (util::get_backtrack_matrix(model_entity, matrix))
							{
								float backtrack_color[3] =
								{
									cfg.player.backtrack_chams_color[0] / 255.0f,
									cfg.player.backtrack_chams_color[1] / 255.0f,
									cfg.player.backtrack_chams_color[2] / 255.0f
								};

								m_renderview()->SetBlend(alpha * alpha_modifier);
								util::color_modulate(backtrack_color, backtrack_material);

								backtrack_material->IncrementReferenceCount(); 
								backtrack_material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, true);

								m_modelrender()->ForcedMaterialOverride(backtrack_material);
								original_fn(m_modelrender(), ctx, state, info, matrix);
								m_modelrender()->ForcedMaterialOverride(nullptr);
							}
						}
					}

					alpha = (float)cfg.player.type[ENEMY].xqz_color.a() / 255.0f;

					float xqz_color[3] =
					{
						cfg.player.type[ENEMY].xqz_color[0] / 255.0f,
						cfg.player.type[ENEMY].xqz_color[1] / 255.0f,
						cfg.player.type[ENEMY].xqz_color[2] / 255.0f
					};

					m_renderview()->SetBlend(alpha * alpha_modifier); 
					util::color_modulate(xqz_color, material);

					material->IncrementReferenceCount();
					material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, true);

					m_modelrender()->ForcedMaterialOverride(material);
					original_fn(m_modelrender(), ctx, state, info, bone_to_world);
					m_modelrender()->ForcedMaterialOverride(nullptr);
					
					alpha = (float)cfg.player.type[ENEMY].chams_color.a() / 255.0f;

					float normal_color[3] =
					{
						cfg.player.type[ENEMY].chams_color[0] / 255.0f,
						cfg.player.type[ENEMY].chams_color[1] / 255.0f,
						cfg.player.type[ENEMY].chams_color[2] / 255.0f
					};

					m_renderview()->SetBlend(alpha * alpha_modifier);
					util::color_modulate(normal_color, material);

					material->IncrementReferenceCount();
					material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, false);

					m_modelrender()->ForcedMaterialOverride(material);
					original_fn(m_modelrender(), ctx, state, info, bone_to_world);
					m_modelrender()->ForcedMaterialOverride(nullptr);

					if (cfg.player.type[ENEMY].animated_material)
					{
						auto animated_material = materials[9];
						
						if (animated_material)
						{
							alpha = (float)cfg.player.type[ENEMY].animated_material_color.a() / 255.0f;

							float animated_color[3] =
							{
								cfg.player.type[ENEMY].animated_material_color[0] / 255.0f,
								cfg.player.type[ENEMY].animated_material_color[1] / 255.0f,
								cfg.player.type[ENEMY].animated_material_color[2] / 255.0f
							};

							m_renderview()->SetBlend(alpha * alpha_modifier);
							util::color_modulate(animated_color, animated_material);

							animated_material->IncrementReferenceCount();
							animated_material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, true);

							m_modelrender()->ForcedMaterialOverride(animated_material);
							original_fn(m_modelrender(), ctx, state, info, bone_to_world);
							m_modelrender()->ForcedMaterialOverride(nullptr);
						}
					}

					if (cfg.player.type[ENEMY].double_material && cfg.player.type[ENEMY].chams_type != 6)
					{
						alpha = (float)cfg.player.type[ENEMY].double_material_color.a() / 255.0f;

						float double_color[3] =
						{
							cfg.player.type[ENEMY].double_material_color[0] / 255.0f,
							cfg.player.type[ENEMY].double_material_color[1] / 255.0f,
							cfg.player.type[ENEMY].double_material_color[2] / 255.0f
						};

						m_renderview()->SetBlend(alpha * alpha_modifier);
						util::color_modulate(double_color, double_material);

						double_material->IncrementReferenceCount();
						double_material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, true);

						m_modelrender()->ForcedMaterialOverride(double_material);
						original_fn(m_modelrender(), ctx, state, info, bone_to_world);
						m_modelrender()->ForcedMaterialOverride(nullptr);
					}

					called_original = true;
				}
				else if (cfg.player.type[ENEMY].chams[PLAYER_CHAMS_VISIBLE])
				{
					auto alpha = (float)cfg.player.backtrack_chams_color.a() / 255.0f;;

					if (cfg.player.backtrack_chams)
					{
						auto backtrack_material = materials[cfg.player.backtrack_chams_material];

						if (backtrack_material && !backtrack_material->IsErrorMaterial())
						{
							matrix3x4_t matrix[MAXSTUDIOBONES];

							if (util::get_backtrack_matrix(model_entity, matrix))
							{
								float backtrack_color[3] =
								{
									cfg.player.backtrack_chams_color[0] / 255.0f,
									cfg.player.backtrack_chams_color[1] / 255.0f,
									cfg.player.backtrack_chams_color[2] / 255.0f
								};

								m_renderview()->SetBlend(alpha * alpha_modifier);
								util::color_modulate(backtrack_color, backtrack_material);

								backtrack_material->IncrementReferenceCount();
								backtrack_material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, true);

								m_modelrender()->ForcedMaterialOverride(backtrack_material);
								original_fn(m_modelrender(), ctx, state, info, matrix);
								m_modelrender()->ForcedMaterialOverride(nullptr);
							}
						}
					}

					alpha = (float)cfg.player.type[ENEMY].chams_color.a() / 255.0f;

					float normal_color[3] =
					{
						cfg.player.type[ENEMY].chams_color[0] / 255.0f,
						cfg.player.type[ENEMY].chams_color[1] / 255.0f,
						cfg.player.type[ENEMY].chams_color[2] / 255.0f
					};

					m_renderview()->SetBlend(alpha * alpha_modifier);
					util::color_modulate(normal_color, material);

					material->IncrementReferenceCount();
					material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, false);

					m_modelrender()->ForcedMaterialOverride(material);
					original_fn(m_modelrender(), ctx, state, info, bone_to_world);
					m_modelrender()->ForcedMaterialOverride(nullptr);

					if (cfg.player.type[ENEMY].animated_material)
					{
						auto animated_material = materials[9];

						if (animated_material)
						{
							alpha = (float)cfg.player.type[ENEMY].animated_material_color.a() / 255.0f;

							float animated_color[3] =
							{
								cfg.player.type[ENEMY].animated_material_color[0] / 255.0f,
								cfg.player.type[ENEMY].animated_material_color[1] / 255.0f,
								cfg.player.type[ENEMY].animated_material_color[2] / 255.0f
							};

							m_renderview()->SetBlend(alpha * alpha_modifier);
							util::color_modulate(animated_color, animated_material);

							animated_material->IncrementReferenceCount();
							animated_material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, false);

							m_modelrender()->ForcedMaterialOverride(animated_material);
							original_fn(m_modelrender(), ctx, state, info, bone_to_world);
							m_modelrender()->ForcedMaterialOverride(nullptr);
						}
					} 

					if (cfg.player.type[ENEMY].double_material && cfg.player.type[ENEMY].chams_type != 6)
					{
						alpha = (float)cfg.player.type[ENEMY].double_material_color.a() / 255.0f;

						float double_color[3] =
						{
							cfg.player.type[ENEMY].double_material_color[0] / 255.0f,
							cfg.player.type[ENEMY].double_material_color[1] / 255.0f,
							cfg.player.type[ENEMY].double_material_color[2] / 255.0f
						};

						m_renderview()->SetBlend(alpha * alpha_modifier);
						util::color_modulate(double_color, double_material);

						double_material->IncrementReferenceCount();
						double_material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, false);

						m_modelrender()->ForcedMaterialOverride(double_material);
						original_fn(m_modelrender(), ctx, state, info, bone_to_world);
						m_modelrender()->ForcedMaterialOverride(nullptr);
					}

					called_original = true;
				}
			}

			if (!called_original)
				return original_fn(m_modelrender(), ctx, state, info, bone_to_world);
		}
		else if (type == TEAM)
		{
			auto material = materials[cfg.player.type[TEAM].chams_type];
			auto double_material = materials[6];

			if (material && double_material && !material->IsErrorMaterial() && !double_material->IsErrorMaterial())
			{
				if (cfg.player.type[TEAM].chams[PLAYER_CHAMS_VISIBLE] && cfg.player.type[TEAM].chams[PLAYER_CHAMS_INVISIBLE])
				{
					auto alpha = (float)cfg.player.type[TEAM].xqz_color.a() / 255.0f;

					float xqz_color[3] =
					{
						cfg.player.type[TEAM].xqz_color[0] / 255.0f,
						cfg.player.type[TEAM].xqz_color[1] / 255.0f,
						cfg.player.type[TEAM].xqz_color[2] / 255.0f
					};

					m_renderview()->SetBlend(alpha);
					util::color_modulate(xqz_color, material);

					material->IncrementReferenceCount();
					material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, true);

					m_modelrender()->ForcedMaterialOverride(material);
					original_fn(m_modelrender(), ctx, state, info, bone_to_world);
					m_modelrender()->ForcedMaterialOverride(nullptr);

					alpha = (float)cfg.player.type[TEAM].chams_color.a() / 255.0f;

					float normal_color[3] =
					{
						cfg.player.type[TEAM].chams_color[0] / 255.0f,
						cfg.player.type[TEAM].chams_color[1] / 255.0f,
						cfg.player.type[TEAM].chams_color[2] / 255.0f
					};

					m_renderview()->SetBlend(alpha);
					util::color_modulate(normal_color, material);

					material->IncrementReferenceCount();
					material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, false);

					m_modelrender()->ForcedMaterialOverride(material);
					original_fn(m_modelrender(), ctx, state, info, bone_to_world);
					m_modelrender()->ForcedMaterialOverride(nullptr);

					if (cfg.player.type[TEAM].animated_material)
					{
						auto animated_material = materials[9];

						if (animated_material)
						{
							alpha = (float)cfg.player.type[TEAM].animated_material_color.a() / 255.0f;

							float animated_color[3] =
							{
								cfg.player.type[TEAM].animated_material_color[0] / 255.0f,
								cfg.player.type[TEAM].animated_material_color[1] / 255.0f,
								cfg.player.type[TEAM].animated_material_color[2] / 255.0f
							};

							m_renderview()->SetBlend(alpha);
							util::color_modulate(animated_color, animated_material);

							animated_material->IncrementReferenceCount();
							animated_material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, true);

							m_modelrender()->ForcedMaterialOverride(animated_material);
							original_fn(m_modelrender(), ctx, state, info, bone_to_world);
							m_modelrender()->ForcedMaterialOverride(nullptr);
						}
					}

					if (cfg.player.type[TEAM].double_material && cfg.player.type[TEAM].chams_type != 6)
					{
						alpha = (float)cfg.player.type[TEAM].double_material_color.a() / 255.0f;

						float double_color[3] =
						{
							cfg.player.type[TEAM].double_material_color[0] / 255.0f,
							cfg.player.type[TEAM].double_material_color[1] / 255.0f,
							cfg.player.type[TEAM].double_material_color[2] / 255.0f
						};

						m_renderview()->SetBlend(alpha);
						util::color_modulate(double_color, double_material);

						double_material->IncrementReferenceCount();
						double_material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, true);

						m_modelrender()->ForcedMaterialOverride(double_material);
						original_fn(m_modelrender(), ctx, state, info, bone_to_world);
						m_modelrender()->ForcedMaterialOverride(nullptr);
					}

					called_original = true;
				}
				else if (cfg.player.type[TEAM].chams[PLAYER_CHAMS_VISIBLE])
				{
					auto alpha = (float)cfg.player.type[TEAM].chams_color.a() / 255.0f;

					float normal_color[3] =
					{
						cfg.player.type[TEAM].chams_color[0] / 255.0f,
						cfg.player.type[TEAM].chams_color[1] / 255.0f,
						cfg.player.type[TEAM].chams_color[2] / 255.0f
					};

					m_renderview()->SetBlend(alpha);
					util::color_modulate(normal_color, material);

					material->IncrementReferenceCount();
					material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, false);

					m_modelrender()->ForcedMaterialOverride(material);
					original_fn(m_modelrender(), ctx, state, info, bone_to_world);
					m_modelrender()->ForcedMaterialOverride(nullptr);

					if (cfg.player.type[TEAM].animated_material)
					{
						auto animated_material = materials[9];

						if (animated_material)
						{
							alpha = (float)cfg.player.type[TEAM].animated_material_color.a() / 255.0f;

							float animated_color[3] =
							{
								cfg.player.type[TEAM].animated_material_color[0] / 255.0f,
								cfg.player.type[TEAM].animated_material_color[1] / 255.0f,
								cfg.player.type[TEAM].animated_material_color[2] / 255.0f
							};

							m_renderview()->SetBlend(alpha);
							util::color_modulate(animated_color, animated_material);

							animated_material->IncrementReferenceCount();
							animated_material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, false);

							m_modelrender()->ForcedMaterialOverride(animated_material);
							original_fn(m_modelrender(), ctx, state, info, bone_to_world);
							m_modelrender()->ForcedMaterialOverride(nullptr);
						}
					}

					if (cfg.player.type[TEAM].double_material && cfg.player.type[TEAM].chams_type != 6)
					{
						alpha = (float)cfg.player.type[TEAM].double_material_color.a() / 255.0f;

						float double_color[3] =
						{
							cfg.player.type[TEAM].double_material_color[0] / 255.0f,
							cfg.player.type[TEAM].double_material_color[1] / 255.0f,
							cfg.player.type[TEAM].double_material_color[2] / 255.0f
						};

						m_renderview()->SetBlend(alpha);
						util::color_modulate(double_color, double_material);

						double_material->IncrementReferenceCount();
						double_material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, false);

						m_modelrender()->ForcedMaterialOverride(double_material);
						original_fn(m_modelrender(), ctx, state, info, bone_to_world);
						m_modelrender()->ForcedMaterialOverride(nullptr);
					}

					called_original = true;
				}
			}

			if (!called_original)
				return original_fn(m_modelrender(), ctx, state, info, bone_to_world);
		}
		else if (m_input()->m_fCameraInThirdPerson)
		{
			auto alpha_modifier = 1.0f;

			if (cfg.player.transparency_in_scope && g_ctx.globals.scoped)
				alpha_modifier = cfg.player.transparency_in_scope_amount;

			auto material = materials[cfg.player.type[LOCAL].chams_type];
			auto double_material = materials[6];

			if (material && double_material && !material->IsErrorMaterial() && !double_material->IsErrorMaterial())
			{
				if (cfg.player.type[LOCAL].chams[PLAYER_CHAMS_VISIBLE] && cfg.player.type[LOCAL].chams[PLAYER_CHAMS_INVISIBLE])
				{
					auto alpha = (float)cfg.player.type[LOCAL].xqz_color.a() / 255.0f * alpha_modifier;

					float xqz_color[3] =
					{
						cfg.player.type[LOCAL].xqz_color[0] / 255.0f,
						cfg.player.type[LOCAL].xqz_color[1] / 255.0f,
						cfg.player.type[LOCAL].xqz_color[2] / 255.0f
					};

					m_renderview()->SetBlend(alpha);
					util::color_modulate(xqz_color, material);

					material->IncrementReferenceCount();
					material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, true);

					m_modelrender()->ForcedMaterialOverride(material);
					original_fn(m_modelrender(), ctx, state, info, bone_to_world);
					m_modelrender()->ForcedMaterialOverride(nullptr);

					alpha = (float)cfg.player.type[LOCAL].chams_color.a() / 255.0f * alpha_modifier;

					float normal_color[3] =
					{
						cfg.player.type[LOCAL].chams_color[0] / 255.0f,
						cfg.player.type[LOCAL].chams_color[1] / 255.0f,
						cfg.player.type[LOCAL].chams_color[2] / 255.0f
					};

					m_renderview()->SetBlend(alpha);
					util::color_modulate(normal_color, material);

					material->IncrementReferenceCount();
					material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, false);

					m_modelrender()->ForcedMaterialOverride(material);
					original_fn(m_modelrender(), ctx, state, info, bone_to_world);
					m_modelrender()->ForcedMaterialOverride(nullptr);

					if (cfg.player.type[LOCAL].animated_material)
					{
						auto animated_material = materials[9];

						if (animated_material)
						{
							auto alpha = (float)cfg.player.type[LOCAL].animated_material_color.a() / 255.0f;

							float animated_color[3] =
							{
								cfg.player.type[LOCAL].animated_material_color[0] / 255.0f,
								cfg.player.type[LOCAL].animated_material_color[1] / 255.0f,
								cfg.player.type[LOCAL].animated_material_color[2] / 255.0f
							};

							m_renderview()->SetBlend(alpha);
							util::color_modulate(animated_color, animated_material);

							animated_material->IncrementReferenceCount();
							animated_material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, true);

							m_modelrender()->ForcedMaterialOverride(animated_material);
							original_fn(m_modelrender(), ctx, state, info, bone_to_world);
							m_modelrender()->ForcedMaterialOverride(nullptr);
						}
					}

					if (cfg.player.type[LOCAL].double_material && cfg.player.type[LOCAL].chams_type != 6)
					{
						alpha = (float)cfg.player.type[LOCAL].double_material_color.a() / 255.0f * alpha_modifier;

						float double_color[3] =
						{
							cfg.player.type[LOCAL].double_material_color[0] / 255.0f,
							cfg.player.type[LOCAL].double_material_color[1] / 255.0f,
							cfg.player.type[LOCAL].double_material_color[2] / 255.0f
						};

						m_renderview()->SetBlend(alpha);
						util::color_modulate(double_color, double_material);

						double_material->IncrementReferenceCount();
						double_material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, true);

						m_modelrender()->ForcedMaterialOverride(double_material);
						original_fn(m_modelrender(), ctx, state, info, bone_to_world);
						m_modelrender()->ForcedMaterialOverride(nullptr);
					}

					called_original = true;
				}
				else if (cfg.player.type[LOCAL].chams[PLAYER_CHAMS_VISIBLE])
				{
					auto alpha = (float)cfg.player.type[LOCAL].chams_color.a() / 255.0f * alpha_modifier;

					float normal_color[3] =
					{
						cfg.player.type[LOCAL].chams_color[0] / 255.0f,
						cfg.player.type[LOCAL].chams_color[1] / 255.0f,
						cfg.player.type[LOCAL].chams_color[2] / 255.0f
					};

					m_renderview()->SetBlend(alpha);
					util::color_modulate(normal_color, material);

					material->IncrementReferenceCount();
					material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, false);

					m_modelrender()->ForcedMaterialOverride(material);
					original_fn(m_modelrender(), ctx, state, info, bone_to_world);
					m_modelrender()->ForcedMaterialOverride(nullptr);

					if (cfg.player.type[LOCAL].animated_material)
					{
						auto animated_material = materials[9];

						if (animated_material)
						{
							alpha = (float)cfg.player.type[LOCAL].animated_material_color.a() / 255.0f * alpha_modifier;

							float animated_color[3] =
							{
								cfg.player.type[LOCAL].animated_material_color[0] / 255.0f,
								cfg.player.type[LOCAL].animated_material_color[1] / 255.0f,
								cfg.player.type[LOCAL].animated_material_color[2] / 255.0f
							};

							m_renderview()->SetBlend(alpha);
							util::color_modulate(animated_color, animated_material);

							animated_material->IncrementReferenceCount();
							animated_material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, false);

							m_modelrender()->ForcedMaterialOverride(animated_material);
							original_fn(m_modelrender(), ctx, state, info, bone_to_world);
							m_modelrender()->ForcedMaterialOverride(nullptr);
						}
					}

					if (cfg.player.type[LOCAL].double_material && cfg.player.type[LOCAL].chams_type != 6)
					{
						alpha = (float)cfg.player.type[LOCAL].double_material_color.a() / 255.0f * alpha_modifier;

						float double_color[3] =
						{
							cfg.player.type[LOCAL].double_material_color[0] / 255.0f,
							cfg.player.type[LOCAL].double_material_color[1] / 255.0f,
							cfg.player.type[LOCAL].double_material_color[2] / 255.0f
						};

						m_renderview()->SetBlend(alpha);
						util::color_modulate(double_color, double_material);

						double_material->IncrementReferenceCount();
						double_material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, false);

						m_modelrender()->ForcedMaterialOverride(double_material);
						original_fn(m_modelrender(), ctx, state, info, bone_to_world);
						m_modelrender()->ForcedMaterialOverride(nullptr);
					}

					called_original = true;
				}
			}

			if (!called_original && cfg.player.layered)
			{
				m_renderview()->SetBlend(alpha_modifier);
				m_renderview()->SetColorModulation(1.0f, 1.0f, 1.0f);

				original_fn(m_modelrender(), ctx, state, info, bone_to_world);
			}

			if (cfg.player.fake_chams_enable)
			{
				if (!local_animations::get().local_data.visualize_lag)
				{
					for (auto& i : g_ctx.globals.fake_matrix)
					{
						i[0][3] += info.origin.x;
						i[1][3] += info.origin.y;
						i[2][3] += info.origin.z;
					}
				}

				auto alpha = (float)cfg.player.fake_chams_color.a() / 255.0f;
				material = materials[cfg.player.fake_chams_type];

				float fake_color[3] =
				{
					cfg.player.fake_chams_color[0] / 255.0f,
					cfg.player.fake_chams_color[1] / 255.0f,
					cfg.player.fake_chams_color[2] / 255.0f
				};

				m_renderview()->SetBlend(alpha);
				util::color_modulate(fake_color, material);

				material->IncrementReferenceCount();
				material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, false);

				m_modelrender()->ForcedMaterialOverride(material);
				original_fn(m_modelrender(), ctx, state, info, g_ctx.globals.fake_matrix);
				m_modelrender()->ForcedMaterialOverride(nullptr);

				if (cfg.player.fake_animated_material)
				{
					auto animated_material = materials[9];

					if (animated_material)
					{
						alpha = (float)cfg.player.fake_animated_material_color.a() / 255.0f;

						float animated_color[3] =
						{
							cfg.player.fake_animated_material_color[0] / 255.0f,
							cfg.player.fake_animated_material_color[1] / 255.0f,
							cfg.player.fake_animated_material_color[2] / 255.0f
						};

						m_renderview()->SetBlend(alpha);
						util::color_modulate(animated_color, animated_material);

						animated_material->IncrementReferenceCount();
						animated_material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, false);

						m_modelrender()->ForcedMaterialOverride(animated_material);
						original_fn(m_modelrender(), ctx, state, info, g_ctx.globals.fake_matrix);
						m_modelrender()->ForcedMaterialOverride(nullptr);
					}
				}

				if (cfg.player.fake_double_material && cfg.player.fake_chams_type != 6 && double_material)
				{
					alpha = (float)cfg.player.fake_double_material_color.a() / 255.0f;

					float double_color[3] =
					{
						cfg.player.fake_double_material_color[0] / 255.0f,
						cfg.player.fake_double_material_color[1] / 255.0f,
						cfg.player.fake_double_material_color[2] / 255.0f
					};

					m_renderview()->SetBlend(alpha);
					util::color_modulate(double_color, double_material);

					double_material->IncrementReferenceCount();
					double_material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, false);

					m_modelrender()->ForcedMaterialOverride(double_material);
					original_fn(m_modelrender(), ctx, state, info, g_ctx.globals.fake_matrix);
					m_modelrender()->ForcedMaterialOverride(nullptr);
				}

				if (!local_animations::get().local_data.visualize_lag)
				{
					for (auto& i : g_ctx.globals.fake_matrix)
					{
						i[0][3] -= info.origin.x;
						i[1][3] -= info.origin.y;
						i[2][3] -= info.origin.z;
					}
				}
			}

			if (!called_original && !cfg.player.layered)
			{
				m_renderview()->SetBlend(alpha_modifier);
				m_renderview()->SetColorModulation(1.0f, 1.0f, 1.0f);

				original_fn(m_modelrender(), ctx, state, info, bone_to_world);
			}
		}
	}
	else if (is_weapon)
	{
		auto alpha = (float)cfg.esp.weapon_chams_color.a() / 255.0f;

		auto material = materials[cfg.esp.weapon_chams_type];
		auto double_material = materials[6];

		if (material && double_material && !material->IsErrorMaterial() && !double_material->IsErrorMaterial())
		{
			float weapon_color[3] =
			{
				cfg.esp.weapon_chams_color[0] / 255.0f,
				cfg.esp.weapon_chams_color[1] / 255.0f,
				cfg.esp.weapon_chams_color[2] / 255.0f
			};

			m_renderview()->SetBlend(alpha);
			util::color_modulate(weapon_color, material);

			material->IncrementReferenceCount();
			material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, false);

			m_modelrender()->ForcedMaterialOverride(material);
			original_fn(m_modelrender(), ctx, state, info, bone_to_world);
			m_modelrender()->ForcedMaterialOverride(nullptr);

			if (cfg.esp.weapon_animated_material)
			{
				auto animated_material = materials[9];

				if (animated_material)
				{
					auto alpha = (float)cfg.esp.weapon_animated_material_color.a() / 255.0f;

					float animated_color[3] =
					{
						cfg.esp.weapon_animated_material_color[0] / 255.0f,
						cfg.esp.weapon_animated_material_color[1] / 255.0f,
						cfg.esp.weapon_animated_material_color[2] / 255.0f
					};

					m_renderview()->SetBlend(alpha);
					util::color_modulate(animated_color, animated_material);

					animated_material->IncrementReferenceCount();
					animated_material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, false);

					m_modelrender()->ForcedMaterialOverride(animated_material);
					original_fn(m_modelrender(), ctx, state, info, bone_to_world);
					m_modelrender()->ForcedMaterialOverride(nullptr);
				}
			}

			if (cfg.esp.weapon_double_material && cfg.esp.weapon_chams_type != 6)
			{
				alpha = (float)cfg.esp.weapon_double_material_color.a() / 255.0f;

				float double_color[3] =
				{
					cfg.esp.weapon_double_material_color[0] / 255.0f,
					cfg.esp.weapon_double_material_color[1] / 255.0f,
					cfg.esp.weapon_double_material_color[2] / 255.0f
				};

				m_renderview()->SetBlend(alpha);
				util::color_modulate(double_color, double_material);

				double_material->IncrementReferenceCount();
				double_material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, false);

				m_modelrender()->ForcedMaterialOverride(double_material);
				original_fn(m_modelrender(), ctx, state, info, bone_to_world);
				m_modelrender()->ForcedMaterialOverride(nullptr);
			}

			called_original = true;
		}

		if (!called_original)
			return original_fn(m_modelrender(), ctx, state, info, bone_to_world);
	}
	else if (weapon_on_back)
	{
		auto alpha = (float)cfg.esp.attachment_chams_color.a() / 255.0f;

		auto material = materials[cfg.esp.attachment_chams_material];
		auto double_material = materials[6];

		if (material && double_material && !material->IsErrorMaterial() && !double_material->IsErrorMaterial())
		{
			float attachment_color[3] =
			{
				cfg.esp.attachment_chams_color[0] / 255.0f,
				cfg.esp.attachment_chams_color[1] / 255.0f,
				cfg.esp.attachment_chams_color[2] / 255.0f
			};

			m_renderview()->SetBlend(alpha);
			util::color_modulate(attachment_color, material);

			material->IncrementReferenceCount();
			material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, false);

			m_modelrender()->ForcedMaterialOverride(material);
			original_fn(m_modelrender(), ctx, state, info, bone_to_world);
			m_modelrender()->ForcedMaterialOverride(nullptr);

			if (cfg.esp.attachment_double_material && cfg.esp.attachment_chams_material != 6)
			{
				alpha = (float)cfg.esp.attachment_double_material_color.a() / 255.0f;

				float double_color[3] =
				{
					cfg.esp.attachment_double_material_color[0] / 255.0f,
					cfg.esp.attachment_double_material_color[1] / 255.0f,
					cfg.esp.attachment_double_material_color[2] / 255.0f
				};

				m_renderview()->SetBlend(alpha);
				util::color_modulate(double_color, double_material);

				double_material->IncrementReferenceCount();
				double_material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, false);

				m_modelrender()->ForcedMaterialOverride(double_material);
				original_fn(m_modelrender(), ctx, state, info, bone_to_world);
				m_modelrender()->ForcedMaterialOverride(nullptr);
			}

			called_original = true;
		}

		if (!called_original)
			return original_fn(m_modelrender(), ctx, state, info, bone_to_world);
	}
	else if (weapon_enemy_hands)
	{
		auto alpha = (float)cfg.esp.attachment_chams_color.a() / 255.0f;

		auto material = materials[cfg.esp.attachment_chams_material];
		auto double_material = materials[6];

		if (material && double_material && !material->IsErrorMaterial() && !double_material->IsErrorMaterial())
		{
			float attachment_color[3] =
			{
				cfg.esp.attachment_chams_color[0] / 255.0f,
				cfg.esp.attachment_chams_color[1] / 255.0f,
				cfg.esp.attachment_chams_color[2] / 255.0f
			};

			m_renderview()->SetBlend(alpha);
			util::color_modulate(attachment_color, material);

			material->IncrementReferenceCount();
			material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, false);

			m_modelrender()->ForcedMaterialOverride(material);
			original_fn(m_modelrender(), ctx, state, info, bone_to_world);
			m_modelrender()->ForcedMaterialOverride(nullptr);

			if (cfg.esp.attachment_double_material && cfg.esp.attachment_chams_material != 6)
			{
				alpha = (float)cfg.esp.attachment_double_material_color.a() / 255.0f;

				float double_color[3] =
				{
					cfg.esp.attachment_double_material_color[0] / 255.0f,
					cfg.esp.attachment_double_material_color[1] / 255.0f,
					cfg.esp.attachment_double_material_color[2] / 255.0f
				};

				m_renderview()->SetBlend(alpha);
				util::color_modulate(double_color, double_material);

				double_material->IncrementReferenceCount();
				double_material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, false);

				m_modelrender()->ForcedMaterialOverride(double_material);
				original_fn(m_modelrender(), ctx, state, info, bone_to_world);
				m_modelrender()->ForcedMaterialOverride(nullptr);
			}

			called_original = true;
		}

		if (!called_original)
			return original_fn(m_modelrender(), ctx, state, info, bone_to_world);
	}
	else if (defuse_kit)
	{
		auto alpha = (float)cfg.esp.attachment_chams_color.a() / 255.0f;

		auto material = materials[cfg.esp.attachment_chams_material];
		auto double_material = materials[6];

		if (material && double_material && !material->IsErrorMaterial() && !double_material->IsErrorMaterial())
		{
			float attachment_color[3] =
			{
				cfg.esp.attachment_chams_color[0] / 255.0f,
				cfg.esp.attachment_chams_color[1] / 255.0f,
				cfg.esp.attachment_chams_color[2] / 255.0f
			};

			m_renderview()->SetBlend(alpha);
			util::color_modulate(attachment_color, material);

			material->IncrementReferenceCount();
			material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, false);

			m_modelrender()->ForcedMaterialOverride(material);
			original_fn(m_modelrender(), ctx, state, info, bone_to_world);
			m_modelrender()->ForcedMaterialOverride(nullptr);

			if (cfg.esp.attachment_double_material && cfg.esp.attachment_chams_material != 6)
			{
				alpha = (float)cfg.esp.attachment_double_material_color.a() / 255.0f;

				float double_color[3] =
				{
					cfg.esp.attachment_double_material_color[0] / 255.0f,
					cfg.esp.attachment_double_material_color[1] / 255.0f,
					cfg.esp.attachment_double_material_color[2] / 255.0f
				};

				m_renderview()->SetBlend(alpha);
				util::color_modulate(double_color, double_material);

				double_material->IncrementReferenceCount();
				double_material->SetMaterialVarFlag(MATERIAL_VAR_IGNOREZ, false);

				m_modelrender()->ForcedMaterialOverride(double_material);
				original_fn(m_modelrender(), ctx, state, info, bone_to_world);
				m_modelrender()->ForcedMaterialOverride(nullptr);
			}

			called_original = true;
		}

		if (!called_original)
			return original_fn(m_modelrender(), ctx, state, info, bone_to_world);
	}
}