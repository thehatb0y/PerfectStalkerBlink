#include <sc2api/sc2_api.h>
#include <sc2api/sc2_coordinator.h>
#include <iostream>
#include "sc2api/sc2_common.h"
#include <sc2api/sc2_interfaces.h>
#include <omp.h>

using namespace sc2;

Point2D choose_blink_position(Point2D unit, Point2D enemy, int blink_range) {
    if (blink_range % 2 == 0) {
        blink_range = blink_range;
    }
    else {
        blink_range = blink_range + 1;
    }
    // Escolhe posição vertical ou horizontal
    if ((unit.x < enemy.x) && ((unit.x - enemy.x) < 3)) {
        return Point2D((unit.x - blink_range), unit.y);
    } else if ((unit.x > enemy.x) && ((unit.x - enemy.x) < 3)) {
        return Point2D((unit.x + blink_range), unit.y);
    } else if ((unit.y < enemy.y) && ((unit.y - enemy.y) < 3)) {
        return Point2D(unit.x, unit.y - blink_range);
    } else if ((unit.y > enemy.y) && ((unit.y - enemy.y) < 3)) {
        return Point2D(unit.x, unit.y + blink_range);
    }
    // Escolhe posição diagonal
    else
    if (unit.x < enemy.x && unit.y < enemy.y) {
        return Point2D((unit.x - (blink_range / 2)), (unit.y - (blink_range / 2)));
    } else if (unit.x < enemy.x && unit.y > enemy.y) {
        return Point2D((unit.x - (blink_range / 2)), (unit.y + (blink_range / 2)));
    } else if (unit.x > enemy.x && unit.y < enemy.y) {
        return Point2D((unit.x + (blink_range / 2)), (unit.y - (blink_range / 2)));
    } else if (unit.x > enemy.x && unit.y > enemy.y) {
        return Point2D((unit.x + (blink_range / 2)), (unit.y + (blink_range / 2)));
    } else {
        return Point2D(unit.x, unit.y);
    }
}

const Unit* get_enemy_unit_with_lowest_health(const Units& enemy_units) {
    const Unit* lowest_health_unit = nullptr;
    float min_health = std::numeric_limits<float>::max();

    for (const auto& enemy : enemy_units) {
        // Verifica se a saúde da unidade inimiga é menor que a saúde mínima encontrada até agora
        if (enemy->health < min_health) {
            min_health = enemy->health;
            lowest_health_unit = enemy;
        }
    }
    // Verifica se a unidade inimiga esta com 100% da vida
    if (lowest_health_unit->health == lowest_health_unit->health_max) {
        // verifica a unidade de tag mais baixa
        for (const auto& enemy : enemy_units) {
            if (enemy->tag < lowest_health_unit->tag) {
                lowest_health_unit = enemy;
            }
        }
        return lowest_health_unit;
    }
    return lowest_health_unit;
}

void StalkerControl(const Unit& stalker, const Units& enemy_units, const ObservationInterface* observation, ActionInterface* actions) {
    // Ataca a unidade inimiga com a menor saúde
    if (stalker.weapon_cooldown <= 0.5) {
        actions->UnitCommand(&stalker, ABILITY_ID::ATTACK_ATTACK, get_enemy_unit_with_lowest_health(enemy_units));
    }
    // Se a habilidade de blink do stalker estiver pronta, use-a
    if (stalker.shield < 40) {
        actions->UnitCommand(&stalker, ABILITY_ID::EFFECT_BLINK, choose_blink_position(stalker.pos, get_enemy_unit_with_lowest_health(enemy_units)->pos, 2));
    }
    // Se a vida do stalker estiver abaixo de 70 e a habilidade de ataque estiver em recarga, mova-se para uma posição contrária ao inimigo
    if (stalker.health < 70 && stalker.weapon_cooldown > 0 && (observation->GetGameLoop() % 15) == 0) {
        actions->UnitCommand(&stalker, ABILITY_ID::MOVE, choose_blink_position(stalker.pos, get_enemy_unit_with_lowest_health(enemy_units)->pos, 1));
    }
}

void UnitsControl(const Units& stalkers, const Units& enemy_units, const ObservationInterface* observation, ActionInterface* actions) {
    // Verifica se há stalkers
    if (!stalkers.empty()) {
        // Verifica se há unidades inimigas
        if (!enemy_units.empty()) {
            // Obtém o número de threads disponíveis
            int num_threads = omp_get_max_threads();
            
            // Paraleliza o loop para cada stalker
            #pragma omp parallel for num_threads(num_threads)
            for (int i = 0; i < stalkers.size(); ++i) {
                // Obtém o ID da thread
                int thread_id = omp_get_thread_num();
                
                // Obtém o stalker atual
                const auto& stalker = stalkers[i];
                
                // Chama a função de controle do stalker
                StalkerControl(*stalker, enemy_units, observation, actions);
                
                // Imprime o número da thread e a iteração
                std::cout << "Thread " << thread_id << " executando iteração " << i << std::endl;
            }
        }
    }
}

class Bot : public Agent {
public:
    virtual void OnGameStart() final {
        std::cout << "Loading" << std::endl;
    }

    // Game steps 
    virtual void OnStep() final {
        // check if step = 0 
        if (Observation()->GetGameLoop() == 0) {
            std::cout << "Game Started" << std::endl;
        }
        // select all stalker units
        Units stalkers = Observation()->GetUnits(Unit::Alliance::Self, IsUnit(UNIT_TYPEID::PROTOSS_STALKER));
        // select all enemy units
        Units enemy_units = Observation()->GetUnits(Unit::Alliance::Enemy);
        // call the stalker control function    
        UnitsControl(stalkers, enemy_units, Observation(), Actions());
    }
};

int main(int argc, char* argv[]) {
    // Create a game coordinator
    Coordinator coordinator;
    // Load settings
    coordinator.LoadSettings(argc, argv);
    // Set realtime
    coordinator.SetRealtime(true);

    Bot bot;
    coordinator.SetParticipants({
        CreateParticipant(Race::Terran, &bot),
        CreateComputer(Race::Zerg)
    });

    // Start the game
    coordinator.LaunchStarcraft();
    // Select Map - MapBelShirVestigeLE
    coordinator.StartGame(sc2::kMapBelShirVestigeLE);
    
    while (coordinator.Update()) {
    }

    return 0;
}