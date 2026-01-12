#include "animation_system.h"
#include "..\ragebot\aim.h"
#include "..\ragebot\antiaim.h"
#include "..\misc\logs.h"

Vector player_t::get_eye_pos() { // Get eye position of the player
    return m_vecOrigin() + m_vecViewOffset();
}

// Инициализация данных для resolver'а
void resolver::initialize(player_t* player, adjust_data* record, const float& goal_feet_yaw, const float& pitch)
{
    this->player = player;
    this->player_record = record;

    original_goal_feet_yaw = math::normalize_yaw(goal_feet_yaw);
    original_pitch = math::normalize_pitch(pitch);

    side = 	RESOLVER_ORIGINAL;
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
void resolver::resolve() // купите пиво, иначе работать не будет
{
    if (!player || !player->is_alive())
        return;

    auto anim_state = player->get_animation_state();
    if (!anim_state)
        return;

    // Устанавливаем начальную сторону
    player_record->side = RESOLVER_FIRST;

    // Проверка на валидность игрока для резолвера
    if (player->m_iTeamNum() == g_ctx.local()->m_iTeamNum() ||
        player->get_move_type() == MOVETYPE_LADDER ||
        player->get_move_type() == MOVETYPE_NOCLIP)
    {
        player_record->side = RESOLVER_FIRST;
        return;
    }

    // Инициализация угла для вычислений
    float foot_yaw = 0.0f;
    BuildMoveYaw(player, foot_yaw);

    // Обновляем слои анимации для цикла и yaw
    auto anim_layers = player->get_animlayers();
    anim_layers[ANIMATION_LAYER_MOVEMENT_MOVE].m_flCycle = player_record->layers[ANIMATION_LAYER_MOVEMENT_MOVE].m_flCycle;
    anim_layers[ANIMATION_LAYER_WHOLE_BODY].m_flCycle = player_record->layers[ANIMATION_LAYER_WHOLE_BODY].m_flCycle;

    // Определение видимости с левой и правой стороны
    bool left_visible = util::visible(g_ctx.globals.eye_pos,
        player->hitbox_position_matrix(HITBOX_HEAD, player_record->leftmatrixes),
        player,
        g_ctx.local());

    bool right_visible = util::visible(g_ctx.globals.eye_pos,
        player->hitbox_position_matrix(HITBOX_HEAD, player_record->rightmatrixes),
        player,
        g_ctx.local());

    // Устанавливаем сторону по видимости или расстоянию
    if (left_visible != right_visible)
    {
        player_record->side = left_visible ? RESOLVER_LEFT : RESOLVER_RIGHT;
    }
    else
    {
        // Определение стороны по расстоянию до головы
        float left_dist = g_ctx.globals.eye_pos.DistTo(player->hitbox_position_matrix(HITBOX_HEAD, player_record->leftmatrixes));
        float right_dist = g_ctx.globals.eye_pos.DistTo(player->hitbox_position_matrix(HITBOX_HEAD, player_record->rightmatrixes));
        player_record->side = (left_dist < right_dist) ? RESOLVER_LEFT : RESOLVER_RIGHT;
    }

    // Устанавливаем конечный угол GoalFeetYaw на основе выбранной стороны
    if (player_record->side == RESOLVER_LEFT)
        anim_state->m_flGoalFeetYaw = math::normalize_yaw(foot_yaw - 60.0f);
    else if (player_record->side == RESOLVER_RIGHT)
        anim_state->m_flGoalFeetYaw = math::normalize_yaw(foot_yaw + 60.0f);
    else
        anim_state->m_flGoalFeetYaw = foot_yaw; // Исходный угол при неопределенной стороне

    // Настройка слоев анимации в зависимости от стороны и состояния игрока
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
