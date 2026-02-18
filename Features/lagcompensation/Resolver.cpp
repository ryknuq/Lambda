#include "animation_system.h"
#include "..\ragebot\aim.h"
#include "..\ragebot\antiaim.h"
#include "..\misc\logs.h"

Vector player_t::get_eye_pos() { // Get eye position of the player
    return m_vecOrigin() + m_vecViewOffset();
}

// Инициализация данных для resolver'а
void resolver::initialize(player_t* e, adjust_data* record, const float& goal_feet_yaw, const float& pitch)
{
    this->player = e;
    this->player_record = record;
    
    // Track previous record for delta analysis
    auto records = &player_records[e->EntIndex()];
    if (records->size() >= 2)
        this->previous_player_record = &records->at(1);
    else
        this->previous_player_record = nullptr;

    original_goal_feet_yaw = math::normalize_yaw(goal_feet_yaw);
    original_pitch = math::normalize_pitch(pitch);

    side = RESOLVER_ORIGINAL;
    fake = false;
}

void resolver::reset()
{
    player = nullptr;
    player_record = nullptr;

    side = RESOLVER_ORIGINAL;
    fake = false;
}

// Основная функция определения и применения углов
void resolver::resolve()
{
    if (!player || !player->is_alive())
        return;

    auto anim_state = player->get_animation_state();
    if (!anim_state)
        return;

    player_record->side = RESOLVER_ORIGINAL;

    // Check validity - skip teammates, ladder, noclip
    if (player->m_iTeamNum() == g_ctx.local()->m_iTeamNum() ||
        player->get_move_type() == MOVETYPE_LADDER ||
        player->get_move_type() == MOVETYPE_NOCLIP)
    {
        player_record->side = RESOLVER_ORIGINAL;
        return;
    }

    // Get velocity and movement state
    float velocity_2d = player->m_vecVelocity().Length2D();
    bool is_moving = velocity_2d > 0.1f;
    
    // Analyze animation layers for better detection
    auto layers = player->get_animlayers();
    
    // Layer 6 = movement, Layer 12 = lean
    float move_weight = layers[ANIMATION_LAYER_MOVEMENT_MOVE].m_flWeight;
    float lean_weight = layers[ANIMATION_LAYER_LEAN].m_flWeight;
    float adjust_weight = layers[ANIMATION_LAYER_ADJUST].m_flWeight; // LBY break layer
    
    // Build movement yaw
    float move_yaw = 0.0f;
    BuildMoveYaw(player, move_yaw);
    
    // MOVING RESOLVER - Priority for moving targets
    if (is_moving && move_weight > 0.1f)
    {
        player_record->moving_resolver_active = true;
        
        // Calculate velocity angle
        float velocity_yaw = atan2(-player->m_vecVelocity().y, -player->m_vecVelocity().x) * (180.0f / M_PI);
        velocity_yaw = math::normalize_yaw(velocity_yaw);
        
        // Check animation layer delta to detect strafing
        float layer_delta = 0.0f;
        if (previous_player_record && previous_player_record->valid())
        {
            layer_delta = layers[ANIMATION_LAYER_MOVEMENT_STRAFECHANGE].m_flCycle - 
                          previous_player_record->layers[ANIMATION_LAYER_MOVEMENT_STRAFECHANGE].m_flCycle;
        }
        
        // Detect strafe direction from layer analysis
        bool strafing_left = layer_delta > 0.01f && lean_weight < -0.2f;
        bool strafing_right = layer_delta > 0.01f && lean_weight > 0.2f;
        
        if (strafing_left)
        {
            player_record->side = RESOLVER_LEFT;
            anim_state->m_flGoalFeetYaw = math::normalize_yaw(velocity_yaw - 35.0f);
        }
        else if (strafing_right)
        {
            player_record->side = RESOLVER_RIGHT;
            anim_state->m_flGoalFeetYaw = math::normalize_yaw(velocity_yaw + 35.0f);
        }
        else
        {
            // Not strafing - use velocity-based resolution
            player_record->side = RESOLVER_ORIGINAL;
            anim_state->m_flGoalFeetYaw = velocity_yaw;
        }
        
        update_animation_layers(player);
        return;
    }
    
    // STANDING RESOLVER - For stationary targets
    player_record->moving_resolver_active = false;
    
    // Check for LBY update (layer 3 weight spike)
    bool lby_updating = adjust_weight > 0.9f;
    
    // Shot history - learn from previous misses
    int miss_count = g_ctx.globals.missed_shots[player->EntIndex()];
    
    // Bruteforce on consecutive misses (after 2+ misses)
    if (miss_count >= 2)
    {
        // Cycle through resolver modes
        static int bruteforce_cycle = 0;
        bruteforce_cycle = (bruteforce_cycle + 1) % 4;
        
        switch (bruteforce_cycle)
        {
        case 0:
            player_record->side = RESOLVER_ZERO;
            anim_state->m_flGoalFeetYaw = player->m_angEyeAngles().y;
            break;
        case 1:
            player_record->side = RESOLVER_LEFT;
            anim_state->m_flGoalFeetYaw = math::normalize_yaw(player->m_angEyeAngles().y - 60.0f);
            break;
        case 2:
            player_record->side = RESOLVER_RIGHT;
            anim_state->m_flGoalFeetYaw = math::normalize_yaw(player->m_angEyeAngles().y + 60.0f);
            break;
        case 3:
            player_record->side = RESOLVER_LOW_FIRST;
            anim_state->m_flGoalFeetYaw = math::normalize_yaw(player->m_angEyeAngles().y - 30.0f);
            break;
        }
        
        update_animation_layers(player);
        return;
    }
    
    // Freestanding - check both sides for visibility
    bool left_visible = util::visible(g_ctx.globals.eye_pos,
        player->hitbox_position_matrix(HITBOX_HEAD, player_record->matrixes_data.first),
        player,
        g_ctx.local());

    bool right_visible = util::visible(g_ctx.globals.eye_pos,
        player->hitbox_position_matrix(HITBOX_HEAD, player_record->matrixes_data.second),
        player,
        g_ctx.local());

    // Priority: Visible side
    if (left_visible != right_visible)
    {
        player_record->side = left_visible ? RESOLVER_LEFT : RESOLVER_RIGHT;
        float delta = left_visible ? -58.0f : 58.0f;
        anim_state->m_flGoalFeetYaw = math::normalize_yaw(player->m_angEyeAngles().y + delta);
    }
    else
    {
        // Both sides visible or both occluded - use distance heuristic
        float left_dist = g_ctx.globals.eye_pos.DistTo(
            player->hitbox_position_matrix(HITBOX_HEAD, player_record->matrixes_data.first));
        float right_dist = g_ctx.globals.eye_pos.DistTo(
            player->hitbox_position_matrix(HITBOX_HEAD, player_record->matrixes_data.second));
        
        player_record->side = (left_dist < right_dist) ? RESOLVER_LEFT : RESOLVER_RIGHT;
        float delta = (left_dist < right_dist) ? -58.0f : 58.0f;
        anim_state->m_flGoalFeetYaw = math::normalize_yaw(player->m_angEyeAngles().y + delta);
    }
    
    update_animation_layers(player);
}

// Обновление анимационных слоев в зависимости от текущей стороны
void resolver::update_animation_layers(player_t* player)
{
    if (!player)
        return;

    auto anim_layers = player->get_animlayers();

    // Устанавливаем значения для слоев на основе движения и состояния
    anim_layers[ANIMATION_LAYER_AIMMATRIX].m_flWeight = (player_record->side == RESOLVER_LEFT) ? 0.5f : 1.0f;
    anim_layers[ANIMATION_LAYER_ADJUST].m_flCycle = 0.0f; // Цикл при изменении направления
    anim_layers[ANIMATION_LAYER_MOVEMENT_STRAFECHANGE].m_flWeight = player->m_vecVelocity().Length2D() > 10.0f ? 0.5f : 0.0f;

    if (player->m_fFlags() & FL_ONGROUND)
    {
        anim_layers[ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB].m_flWeight = 1.0f;
        anim_layers[ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL].m_flWeight = 0.0f;
    }
    else
    {
        anim_layers[ANIMATION_LAYER_MOVEMENT_JUMP_OR_FALL].m_flWeight = 1.0f;
        anim_layers[ANIMATION_LAYER_MOVEMENT_LAND_OR_CLIMB].m_flWeight = 0.0f;
    }

    // Обнуление слоев, не требующих изменений
    anim_layers[ANIMATION_LAYER_FLINCH].m_flWeight = 0.0f;
    anim_layers[ANIMATION_LAYER_FLASHED].m_flWeight = 0.0f;
}

// Вспомогательная функция для настройки yaw и других параметров движения
void resolver::BuildMoveYaw(player_t* player, float& foot_yaw)
{
    if (!player)
        return;

    float velocity = player->m_vecVelocity().Length2D();
    if (velocity > 0.1f)
    {
        foot_yaw = atan2(-player->m_vecVelocity().y, -player->m_vecVelocity().x) * (180.0f / M_PI);
        foot_yaw = math::normalize_yaw(foot_yaw);
    }
    else
    {
        foot_yaw = player->m_angEyeAngles().y;
    }
}
