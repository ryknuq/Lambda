#include "local_animations.h"


void local_animations::run(ClientFrameStage_t stage) // Шиза
{
    auto local = g_ctx.local();
    auto* animstate = local->get_animation_state();

    // Проверка на nullptr для animstate
    if (!animstate)
    {
        std::cout << "Error: Animation state is null!" << std::endl;
        return;
    }

    if (!fakelag::get().condition && key_binds::get().get_key_bind_state(20))
    {
        if (stage == FRAME_NET_UPDATE_END)
        {
            fake_server_update = false;

            if (local->m_flSimulationTime() != fake_simulation_time)
            {
                fake_server_update = true;
                fake_simulation_time = local->m_flSimulationTime();
            }

            // Сбрасываем вес и цикл для определенных анимаций
            reset_animation_layers();

            update_fake_animations();
        }
        else if (stage == FRAME_RENDER_START)
        {
            // Сбрасываем вес и цикл для определенных анимаций
            reset_animation_layers();

            update_local_animations(animstate);
        }
    }
    else if (stage == FRAME_RENDER_START)
    {
        real_server_update = false;
        fake_server_update = false;

        if (local->m_flSimulationTime() != real_simulation_time || local->m_flSimulationTime() != fake_simulation_time)
        {
            real_server_update = fake_server_update = true;
            real_simulation_time = fake_simulation_time = local->m_flSimulationTime();
        }

        // Сбрасываем вес и цикл для определенных анимаций
        reset_animation_layers();

        update_fake_animations();
        update_local_animations(animstate);
    }

    handle_landing_animation();
    handle_walk_type();
}

void local_animations::reset_animation_layers()
{
    auto local = g_ctx.local();
    auto animlayers = local->get_animlayers();

    animlayers[3].m_flWeight = 0.0f;
    animlayers[3].m_flCycle = 0.0f;
    animlayers[12].m_flWeight = 0.0f;
}

void local_animations::handle_landing_animation()
{
    if (cfg.antiaim.pitch_zero_on_land && g_ctx.local()->get_animation_state()->m_bInHitGroundAnimation)
    {
        if (g_ctx.local()->get_animation_state()->m_flHeadHeightOrOffsetFromHittingGroundAnimation)
        {
            g_ctx.local()->m_flPoseParameter()[12] = 0.5f;
        }
        else
        {
            g_ctx.local()->m_flPoseParameter()[12] = 1.0f;
        }
    }
}

void local_animations::handle_walk_type()
{
    if (cfg.antiaim.walk_type == 1)
        g_ctx.local()->m_flPoseParameter()[0] = 1.0f; // backward legs
    else if (cfg.antiaim.walk_type == 2)
        g_ctx.local()->m_flPoseParameter()[7] = 0.0f; // moon walk
}

void local_animations::update_fake_animations()
{
    bool alloc = !local_data.animstate;
    bool change = !alloc && handle != &g_ctx.local()->GetRefEHandle();
    bool reset = !alloc && !change && g_ctx.local()->m_flSpawnTime() != spawntime;

    if (change)
    {
        if (local_data.animstate)
        {
            m_memalloc()->Free(local_data.animstate);
        }
    }

    if (reset)
    {
        util::reset_state(local_data.animstate);
        spawntime = g_ctx.local()->m_flSpawnTime();
    }

    if (alloc || change)
    {
        local_data.animstate = (c_baseplayeranimationstate*)m_memalloc()->Alloc(sizeof(c_baseplayeranimationstate));

        if (local_data.animstate)
        {
            util::create_state(local_data.animstate, g_ctx.local());
        }
        else
        {
            std::cout << "Error: Failed to allocate memory for animstate!" << std::endl;
            return;
        }

        handle = (CBaseHandle*)&g_ctx.local()->GetRefEHandle();
        spawntime = g_ctx.local()->m_flSpawnTime();
    }

    if (!alloc && !change && !reset && fake_server_update)
    {
        // Копирование данных для анимаций
        save_animation_state();

        // Сохранение состояния фреймтайма и текущего времени
        auto backup_frametime = m_globals()->m_frametime;
        auto backup_curtime = m_globals()->m_curtime;

        m_globals()->m_frametime = m_globals()->m_intervalpertick;
        m_globals()->m_curtime = g_ctx.local()->m_flSimulationTime();

        local_data.animstate->m_pBaseEntity = g_ctx.local();
        util::update_state(local_data.animstate, local_animations::get().local_data.fake_angles);

        local_data.animstate->m_bInHitGroundAnimation = false;
        local_data.animstate->m_fLandingDuckAdditiveSomething = 0.0f;
        local_data.animstate->m_flHeadHeightOrOffsetFromHittingGroundAnimation = 1.0f;

        g_ctx.local()->setup_bones_fixed(g_ctx.globals.fake_matrix, BONE_USED_BY_ANYTHING);

        if (!local_data.visualize_lag)
        {
            for (auto& i : g_ctx.globals.fake_matrix)
            {
                i[0][3] -= g_ctx.local()->GetRenderOrigin().x;
                i[1][3] -= g_ctx.local()->GetRenderOrigin().y;
                i[2][3] -= g_ctx.local()->GetRenderOrigin().z;
            }
        }

        // Восстановление состояния фреймтайма и времени
        m_globals()->m_frametime = backup_frametime;
        m_globals()->m_curtime = backup_curtime;

        // Восстановление поз и слоев анимации
        restore_animation_state();
    }
}

void local_animations::save_animation_state()
{
    // Сохранение текущих поз и слоев анимации
    memcpy(pose_parameter, &g_ctx.local()->m_flPoseParameter(), sizeof(float) * 24);
    memcpy(layers, g_ctx.local()->get_animlayers(), sizeof(AnimationLayer) * g_ctx.local()->animlayer_count());
}

void local_animations::restore_animation_state()
{
    // Восстановление поз и слоев анимации
    memcpy(&g_ctx.local()->m_flPoseParameter(), pose_parameter, sizeof(float) * 24);
    memcpy(g_ctx.local()->get_animlayers(), layers, sizeof(AnimationLayer) * g_ctx.local()->animlayer_count());
}

void local_animations::update_local_animations(c_baseplayeranimationstate* animstate)
{
    if (tickcount != m_globals()->m_tickcount)
    {
        memcpy(layers, g_ctx.local()->get_animlayers(), sizeof(AnimationLayer) * g_ctx.local()->animlayer_count());
        animstate->m_iLastClientSideAnimationUpdateFramecount = 0;
        util::update_state(animstate, local_animations::get().local_data.fake_angles);

        if (!m_clientstate()->iChokedCommands)
        {
            abs_angles = animstate->m_flGoalFeetYaw;
            memcpy(pose_parameter, &g_ctx.local()->m_flPoseParameter(), sizeof(float) * 24);
        }

        tickcount = m_globals()->m_tickcount;
    }
    else
        animstate->m_iLastClientSideAnimationUpdateFramecount = m_globals()->m_framecount;

    animstate->m_flGoalFeetYaw = antiaim::get().condition(g_ctx.get_command()) ? abs_angles : local_animations::get().local_data.real_angles.y;
    animstate->m_flFeetYawRate = 0.f;

    g_ctx.local()->set_abs_angles(Vector(0.0f, abs_angles, 0.0f));

    // Восстановление анимационных данных
    restore_animation_state();
}
