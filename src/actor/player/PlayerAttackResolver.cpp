#include "actor/player/PlayerAttackResolver.h"

#include "actor/Enemy.h"
#include "actor/Player.h"
#include "actor/player/PlayerCombat.h"
#include "actor/player/PlayerJewelGauge.h"
#include "actor/player/PlayerMovement.h"
#include "actor/player/PlayerStatus.h"
#include "system/AudioSystem.h"

void PlayerAttackResolver::ResolveAttack(Player& player, PlayerMovement& movement, PlayerStatus& status,
                                         PlayerCombat& combat, const std::vector<Enemy*>& hitEnemies,
                                         float deltaTime) const
{
    if (hitEnemies.empty()) {
        combat.StartAfterAttackReaction(player, movement, status);
        player.GetGame()->GetAudioSystem()->PlaySE("attack_miss_se");

        if (combat.GetAttackComboIndex() != 3) {
            return;
        }

        combat.ResetAttackComboIndex();
        return;
    }

    if (combat.GetAttackKind() != PlayerAttackKind::Strong) {
        player.GetGame()->OnPlayerAttackHit(movement.GetPlayerNum());
        combat.StartAfterAttackReaction(player, movement, status);

        if (player.GetOnGround()) {
            for (Enemy* enemy : hitEnemies) {
                enemy->ApplyDamage(combat.GetAttack(), &player);
            }
        } else {
            bool isHit = false;

            for (Enemy* enemy : hitEnemies) {
                if (enemy->GetOnGround()) {
                    continue;
                }

                enemy->ApplyDamage(combat.GetAttack(), &player);
                isHit = true;
            }

            if (isHit) {
                player.GetGame()->GetAudioSystem()->PlaySE("attack_se");
            } else {
                player.GetGame()->GetAudioSystem()->PlaySE("attack_miss_se");
            }

            return;
        }

        if (combat.GetAttackComboIndex() != 3) {
            player.GetGame()->GetAudioSystem()->PlaySE("attack_se");
            return;
        }

        combat.ResetAttackComboIndex();
        player.GetGame()->GetAudioSystem()->PlaySE("destroy_se");

        for (Enemy* enemy : hitEnemies) {
            if (enemy->GetOnGround()) {
                enemy->ApplyBreak(deltaTime);
            }
        }

        return;
    }

    player.GetGame()->GetAudioSystem()->PlaySE("attack_air_se");
    combat.StartTiredLock(status, movement, 5.0f);

    for (Enemy* enemy : hitEnemies) {
        enemy->SetIsStrongAttacked(true);
        enemy->ApplyDamage(combat.GetAttack(), &player);
        combat.SetStrongAttackHit(true);
    }
}

void PlayerAttackResolver::ResolveSpecialAttack(Player& player, PlayerJewelGauge& jewelGauge,
                                                const std::vector<Enemy*>& hitEnemies, float deltaTime) const
{
    for (Enemy* enemy : hitEnemies) {
        if (enemy->GetIsDead()) {
            continue;
        }

        if (enemy->GetOnGround()) {
            while (enemy->GetBreakCount()) {
                enemy->ApplyBreak(deltaTime);
            }
        }

        if (enemy->GetCanCountered()) {
            enemy->ApplyDamage(600, &player);
            enemy->FlipCanCountered();
            jewelGauge.RestoreFull();
            player.GetGame()->GetAudioSystem()->PlaySE("just_attack_se");
        } else {
            enemy->ApplyDamage(300, &player);
        }
    }
}
