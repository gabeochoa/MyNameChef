#pragma once

#include "../components/render_order.h"
#include "../components/has_shader.h"
#include "../components/transform.h"
#include "../game_state_manager.h"
#include "../shader_library.h"
#include "../shader_types.h"
#include <afterhours/ah.h>
#include <afterhours/src/plugins/color.h>
using namespace afterhours;

struct RenderSpritesWithShaders
    : System<Transform, afterhours::texture_manager::HasSprite, HasShader,
             HasColor, HasRenderOrder> {
  virtual bool should_run(float) override {
    auto &gsm = GameStateManager::get();
    return gsm.is_game_active();
  }

  struct EntityRenderData {
    const Transform *transform;
    const afterhours::texture_manager::HasSprite *hasSprite;
    const HasColor *hasColor;
    const HasShader *hasShader;
  };

  // Cache for batching entities by shader type
  mutable std::unordered_map<ShaderType, std::vector<EntityRenderData>>
      shader_batches;
  mutable bool uniforms_updated_this_frame = false;
  mutable float last_time = 0.0f;
  mutable vec2 last_resolution = {0, 0};

  virtual void
  for_each_with(const Entity & /* entity */, const Transform &transform,
                const afterhours::texture_manager::HasSprite &hasSprite,
                const HasShader &hasShader, const HasColor &hasColor,
                const HasRenderOrder &render_order, float) const override {
    // Check if this entity should render on the current screen
    auto &gsm = GameStateManager::get();
    RenderScreen current_screen = get_current_render_screen(gsm);
    
    if (!render_order.should_render_on_screen(current_screen)) {
      return;
    }

    // Collect entity data for batching instead of rendering immediately
    if (hasShader.shaders.empty()) {
      return;
    }

    ShaderType primary_shader = hasShader.shaders[0];
    if (!ShaderLibrary::get().contains(primary_shader)) {
      return;
    }

    shader_batches[primary_shader].push_back(
        {&transform, &hasSprite, &hasColor, &hasShader});
  }

  virtual void once(float) const override {
    // Render all batches after collecting all entities
    render_all_batches();
    shader_batches.clear();
    uniforms_updated_this_frame = false;
  }

private:
  RenderScreen get_current_render_screen(const GameStateManager &gsm) const {
    switch (gsm.active_screen) {
      case GameStateManager::Screen::Shop:
        return RenderScreen::Shop;
      case GameStateManager::Screen::Battle:
        return RenderScreen::Battle;
      case GameStateManager::Screen::Results:
        return RenderScreen::Results;
      case GameStateManager::Screen::Main:
      case GameStateManager::Screen::Settings:
      default:
        return RenderScreen::All; // Default to all screens for other screens
    }
  }

  void render_all_batches() const {
    for (auto &[shader_type, entities] : shader_batches) {
      if (entities.empty())
        continue;

      render_shader_batch(shader_type, entities);
    }
  }

  void
  render_shader_batch(ShaderType shader_type,
                      const std::vector<EntityRenderData> &entities) const {
    const auto &shader = ShaderLibrary::get().get(shader_type);

    // Get spritesheet once per batch
    auto *spritesheet_component = EntityHelper::get_singleton_cmp<
        afterhours::texture_manager::HasSpritesheet>();
    if (!spritesheet_component) {
      return;
    }

    raylib::Texture2D sheet = spritesheet_component->texture;

    // Render each entity with its own shader state
    for (const auto &entity_data : entities) {
      raylib::BeginShaderMode(shader);

      // Update common uniforms once per entity
      update_shader_uniforms(shader, shader_type);

      // Render the entity with its specific uniforms
      render_single_entity(entity_data, sheet, shader, shader_type);

      raylib::EndShaderMode();
    }
  }

  void update_shader_uniforms(const raylib::Shader &shader,
                              ShaderType /* shader_type */) const {
    float current_time = static_cast<float>(raylib::GetTime());

    // Update time uniform if it changed
    if (current_time != last_time) {
      int timeLoc = raylib::GetShaderLocation(shader, "time");
      if (timeLoc != -1) {
        raylib::SetShaderValue(shader, timeLoc, &current_time,
                               raylib::SHADER_UNIFORM_FLOAT);
      }
      last_time = current_time;
    }

    // Update resolution uniform if it changed
    auto rezCmp = EntityHelper::get_singleton_cmp<
        window_manager::ProvidesCurrentResolution>();
    if (rezCmp) {
      vec2 current_resolution = {
          static_cast<float>(rezCmp->current_resolution.width),
          static_cast<float>(rezCmp->current_resolution.height)};
      if (current_resolution.x != last_resolution.x ||
          current_resolution.y != last_resolution.y) {
        int rezLoc = raylib::GetShaderLocation(shader, "resolution");
        if (rezLoc != -1) {
          raylib::SetShaderValue(shader, rezLoc, &current_resolution,
                                 raylib::SHADER_UNIFORM_VEC2);
        }
        last_resolution = current_resolution;
      }
    }

    // Update UV bounds once per batch (they're the same for all sprites)
    auto *spritesheet_component = EntityHelper::get_singleton_cmp<
        afterhours::texture_manager::HasSpritesheet>();
    if (spritesheet_component) {
      // Using per-entity frames; default UVs to full sheet. If a shader needs
      // exact UVs, compute them per-entity from hasSprite.frame.
      float uvMin[2] = {0.f, 0.f};
      float uvMax[2] = {1.f, 1.f};

      int uvMinLoc = raylib::GetShaderLocation(shader, "uvMin");
      if (uvMinLoc != -1) {
        raylib::SetShaderValue(shader, uvMinLoc, uvMin,
                               raylib::SHADER_UNIFORM_VEC2);
      }
      int uvMaxLoc = raylib::GetShaderLocation(shader, "uvMax");
      if (uvMaxLoc != -1) {
        raylib::SetShaderValue(shader, uvMaxLoc, uvMax,
                               raylib::SHADER_UNIFORM_VEC2);
      }
    }
  }

  void render_single_entity(const EntityRenderData &entity_data,
                            const raylib::Texture2D &sheet,
                            const raylib::Shader &shader,
                            ShaderType shader_type) const {
    const auto &transform = *entity_data.transform;
    const auto &hasSprite = *entity_data.hasSprite;
    const auto &hasColor = *entity_data.hasColor;
    const auto &hasShader = *entity_data.hasShader;

    // Update per-entity uniforms
    update_per_entity_uniforms(shader, hasShader, hasColor, transform,
                               shader_type);

    // Calculate rendering parameters
    float dest_width = hasSprite.frame.width * hasSprite.scale;
    float dest_height = hasSprite.frame.height * hasSprite.scale;

    float offset_x = 0.0f; // SPRITE_OFFSET_X
    float offset_y = 0.0f; // SPRITE_OFFSET_Y

    float rotated_x = offset_x * cosf(transform.angle * M_PI / 180.f) -
                      offset_y * sinf(transform.angle * M_PI / 180.f);
    float rotated_y = offset_x * sinf(transform.angle * M_PI / 180.f) +
                      offset_y * cosf(transform.angle * M_PI / 180.f);

    // Render the sprite
    raylib::DrawTexturePro(
        sheet, hasSprite.frame,
        Rectangle{
            transform.position.x + transform.size.x / 2.f + rotated_x,
            transform.position.y + transform.size.y / 2.f + rotated_y,
            dest_width,
            dest_height,
        },
        vec2{dest_width / 2.f, dest_height / 2.f}, transform.angle,
        raylib::WHITE);
  }

  void update_per_entity_uniforms(const raylib::Shader &shader,
                                  const HasShader & /*hasShader*/,
                                  const HasColor &hasColor,
                                  const Transform & /*transform*/,
                                  ShaderType /*shader_type*/) const {
    // Update entity-specific uniforms
    raylib::Color entityColor = hasColor.color();
    float entityColorF[4] = {
        entityColor.r / 255.0f,
        entityColor.g / 255.0f,
        entityColor.b / 255.0f,
        entityColor.a / 255.0f,
    };
    int entityColorLoc = raylib::GetShaderLocation(shader, "entityColor");
    if (entityColorLoc != -1) {
      raylib::SetShaderValue(shader, entityColorLoc, entityColorF,
                             raylib::SHADER_UNIFORM_VEC4);
    } else {
      log_warn("entityColor uniform location not found in shader!");
    }
  }
};
