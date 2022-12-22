from typing import Dict, Iterable, List, Optional, Set, Tuple, Union
from sc2.client import Client
from sc2 import maps
from sc2.bot_ai import BotAI
from sc2.data import Difficulty, Race
from sc2.ids.ability_id import AbilityId
from sc2.ids.unit_typeid import UnitTypeId
from sc2.main import run_game
from sc2.player import Bot, Computer

from sc2.position import Point2, Point3
from sc2.unit import Unit

class BlinkSTalker(BotAI):
    async def on_step(self, iteration):
        if iteration == 0:
            await self.chat_send("(probe)(pylon)(cannon)(cannon)(gg)")

        stalkers = self.units(UnitTypeId.STALKER)
        targets = (self.enemy_units | self.enemy_structures).filter(lambda unit: unit.can_be_attacked)

        if len(stalkers)!=0:
            for stalker in stalkers:
                if len(targets)!=0:
                    stalker.attack(choose_enemy(targets))
                    if stalker.shield == 0 and stalker.is_idle == False:
                        if await self.can_cast(stalker, AbilityId.EFFECT_BLINK_STALKER, only_check_energy_and_cooldown=True):
                            stalker(AbilityId.EFFECT_BLINK_STALKER, choose_blink_position(stalker, choose_enemy(targets), 2))
                        else:
                            if stalker.health < 80 and stalker.is_idle == False and stalker.weapon_ready == False and iteration % 3 == 0:
                                stalker.move(choose_blink_position(stalker, choose_enemy(targets), 2))

def choose_enemy(enemys):
    minLife = 0
    for enemy in enemys:
        if minLife == 0:
            minLife = enemy
        if enemy.health < minLife.health:
            minLife = enemy
    return minLife

def choose_blink_position(unit, enemy, blink_range):

    if blink_range % 2 == 0:
        blink_range = blink_range
    else:
        blink_range = blink_range + 1

    if (unit.position.x < enemy.position.x) and ((unit.position.x - enemy.position.x) < 3):
        return Point2((unit.position.x-(blink_range),unit.position.y))
    elif (unit.position.x > enemy.position.x) and ((unit.position.x - enemy.position.x) < 3):
        return Point2((unit.position.x+(blink_range),unit.position.y))
    elif (unit.position.y < enemy.position.y) and ((unit.position.y - enemy.position.y) < 3):
        return Point2((unit.position.x,unit.position.y-(blink_range)))
    elif (unit.position.y < enemy.position.y) and ((unit.position.y - enemy.position.y) < 3):
        return Point2((unit.position.x,unit.position.y+(blink_range)))   

    if unit.position.x < enemy.position.x and unit.position.y < enemy.position.y:
        return Point2((unit.position.x-(blink_range/2),unit.position.y-(blink_range/2)))
    elif unit.position.x < enemy.position.x and unit.position.y > enemy.position.y:
        return Point2((unit.position.x-(blink_range/2),unit.position.y+(blink_range/2)))
    elif unit.position.x > enemy.position.x and unit.position.y < enemy.position.y:
        return Point2((unit.position.x+(blink_range/2),unit.position.y-(blink_range/2)))
    elif unit.position.x > enemy.position.x and unit.position.y > enemy.position.y:
        return Point2((unit.position.x+(blink_range/2),unit.position.y+(blink_range/2)))
    else:
        return Point2((unit.position.x,unit.position.y))

def main():
    run_game(
        maps.get("EditorTest"),
        [Bot(Race.Protoss, BlinkSTalker(), name="BlinkSTalker"),
         Computer(Race.Protoss, Difficulty.Medium)],
        realtime=True,
    )

if __name__ == "__main__":
    main()
