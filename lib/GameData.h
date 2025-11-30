#ifndef GAME_DATA_H
#define GAME_DATA_H

#include "GameTypes.h"

// --- PARTY DEFAULTS ---
inline std::vector<Combatant> INITIAL_PARTY()
{
    std::vector<Combatant> party;

    Combatant joker{};
    joker.id = 0;
    joker.name = "Joker";
    joker.texturePath = "assets/player_joker.png";
    joker.currentHp = joker.maxHp = 100;
    joker.currentSp = joker.maxSp = 50;
    joker.attack = 15; joker.defense = 10; joker.speed = 12;
    joker.isAlive = true; joker.isDown = false; joker.hasActed = false;
    joker.skills = { {"Eiha", 4, 30, CURSE, true}, {"Gun", 2, 20, GUN, false} };
    joker.weaknesses = {};
    party.push_back(joker);

    Combatant skull{};
    skull.id = 1;
    skull.name = "Skull";
    skull.texturePath = "assets/player_skull.png";
    skull.currentHp = skull.maxHp = 150;
    skull.currentSp = skull.maxSp = 30;
    skull.attack = 20; skull.defense = 15; skull.speed = 8;
    skull.isAlive = true; skull.isDown = false; skull.hasActed = false;
    skull.skills = { {"Lunge", 5, 25, PHYS, false}, {"Zio", 4, 25, ELEC, true} };
    skull.weaknesses = { WIND };
    party.push_back(skull);

    Combatant mona{};
    mona.id = 2;
    mona.name = "Mona";
    mona.texturePath = "assets/player_mona.png";
    mona.currentHp = mona.maxHp = 80;
    mona.currentSp = mona.maxSp = 60;
    mona.attack = 12; mona.defense = 8; mona.speed = 15;
    mona.isAlive = true; mona.isDown = false; mona.hasActed = false;
    mona.skills = { {"Garu", 4, 20, WIND, true}, {"Dia", 4, -30, ELEMENT_NONE, true} };
    mona.weaknesses = { ELEC };
    party.push_back(mona);

    Combatant noir{};
    noir.id = 3;
    noir.name = "Noir";
    noir.texturePath = "assets/player_noir.png";
    noir.currentHp = noir.maxHp = 110;
    noir.currentSp = noir.maxSp = 45;
    noir.attack = 18; noir.defense = 12; noir.speed = 10;
    noir.isAlive = true; noir.isDown = false; noir.hasActed = false;
    noir.skills = { {"Psi", 4, 35, PSI, true}, {"Mapsi", 8, 25, PSI, true} };
    noir.weaknesses = { NUKE };
    party.push_back(noir);

    return party;
}

// --- ENEMY DATABASE ---
// Helper to get an enemy by Type/ID
inline Combatant getEnemyData(int id) {
    Combatant enemy;
    enemy.id = id;
    switch(id) {
        case 0: // Pixie
            enemy.name = "Pixie";
            enemy.maxHp = 50; enemy.currentHp = 50;
            enemy.attack = 10;
            enemy.weaknesses = { GUN, ICE, CURSE };
            break;
        case 1: // Jack Frost
            enemy.name = "Jack Frost";
            enemy.maxHp = 120; enemy.currentHp = 120;
            enemy.attack = 15;
            enemy.weaknesses = { FIRE };
            break;
        default: // Shadow
            enemy.name = "Shadow";
            enemy.maxHp = 30; enemy.currentHp = 30;
            enemy.attack = 8;
            break;
    }
    return enemy;
}

#endif // GAME_DATA_H