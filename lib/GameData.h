#ifndef GAME_DATA_H
#define GAME_DATA_H

#include "GameTypes.h"

// Helper to construct Equipment without aggregate initialization issues
inline Equipment MakeEquipment(const std::string& name,
                               EquipmentType type,
                               int attackPower,
                               int defensePower,
                               int magazineSize,
                               Element element,
                               const std::string& description) {
    Equipment e;
    e.name = name;
    e.type = type;
    e.attackPower = attackPower;
    e.defensePower = defensePower;
    e.magazineSize = magazineSize;
    e.element = element;
    e.description = description;
    return e;
}

// --- PARTY DEFAULTS ---
// --- ITEMS DATABASE ---
const Item ITEM_MEDICINE = { "Medicine", "Restores 50 HP", 50, false, false, true };
const Item ITEM_SNUFF_SOUL = { "Snuff Soul", "Restores 20 SP", 20, true, false, true };
const Item ITEM_REVIVAL_BEAD = { "Revival Bead", "Revives ally with 50% HP", 50, false, true, true };

// --- EQUIPMENT DATABASE ---
// Joker
const Equipment WEP_PARADISE_LOST = MakeEquipment("Paradise Lost", EQUIP_MELEE, 5, 0, 0, PHYS, "Joker's Dagger");
const Equipment GUN_TKACHEV = MakeEquipment("Tkachev", EQUIP_GUN, 8, 0, 8, GUN, "Basic Pistol");
const Equipment ARM_TUXEDO = MakeEquipment("Phantom Suit", EQUIP_ARMOR, 0, 5, 0, ELEMENT_NONE, "Stylish Thief Gear");

// Skull
const Equipment WEP_PIPE = MakeEquipment("Iron Pipe", EQUIP_MELEE, 8, 0, 0, PHYS, "Heavy blunt object");
const Equipment GUN_SHOTGUN = MakeEquipment("Shotgun", EQUIP_GUN, 12, 0, 4, GUN, "Short range blaster");

// Mona
const Equipment WEP_SWORD = MakeEquipment("Scimitar", EQUIP_MELEE, 4, 0, 0, PHYS, "Curved blade");
const Equipment GUN_SLINGSHOT = MakeEquipment("Slingshot", EQUIP_GUN, 6, 0, 5, GUN, "Toy but deadly");

// Noir
const Equipment WEP_AXE = MakeEquipment("Battle Axe", EQUIP_MELEE, 10, 0, 0, PHYS, "Heavy elegant axe");
const Equipment GUN_LAUNCHER = MakeEquipment("Grenade Launcher", EQUIP_GUN, 15, 0, 1, GUN, "One shot wonder");

// --- PARTY DEFAULTS ---
// --- PERSONA DATABASE ---
inline std::vector<Persona> INITIAL_PERSONAS() {
    std::vector<Persona> list;

    // 1. Arsene (Starter) - High Speed/Attack, Weak to Ice/Bless
    Persona arsene;
    arsene.name = "Arsene";
    arsene.baseAttack = 12; 
    arsene.baseDefense = 5; 
    arsene.skills = { {"Eiha", 4, 20, CURSE, true}, {"Cleave", 6, 25, PHYS, false} };
    arsene.weaknesses = { ICE, BLESS };
    list.push_back(arsene);

    // 2. Pixie (Healer) - Low Stats, Good Skills, Weak to Gun/Ice
    Persona pixie;
    pixie.name = "Pixie";
    pixie.baseAttack = 8; 
    pixie.baseDefense = 4; 
    pixie.skills = { {"Zio", 4, 15, ELEC, true}, {"Dia", 3, -30, ELEMENT_NONE, true} };
    pixie.weaknesses = { GUN, ICE };
    list.push_back(pixie);

    // 3. Jack Frost (Mage) - Balanced, Weak to Fire
    Persona jack;
    jack.name = "Jack Frost";
    jack.baseAttack = 11; 
    jack.baseDefense = 8; 
    jack.skills = { {"Bufu", 4, 25, ICE, true}, {"Mabufu", 10, 20, ICE, true} };
    jack.weaknesses = { FIRE };
    list.push_back(jack);

    return list;
}
// --- PARTY DEFAULTS ---
inline std::vector<Combatant> INITIAL_PARTY()
{
    std::vector<Combatant> party;

    Combatant joker{};
    joker.id = 0; joker.name = "Joker"; joker.texturePath = "assets/player_joker.png";
    joker.currentHp = joker.maxHp = 130;
    joker.currentSp = joker.maxSp = 60;
    joker.baseAttack = 14; joker.baseDefense = 8; // speed removed
    joker.meleeWeapon = WEP_PARADISE_LOST;
    joker.gunWeapon = GUN_TKACHEV;
    joker.armor = ARM_TUXEDO;
    joker.currentAmmo = joker.gunWeapon.magazineSize;
    joker.skills = { {"Eiha", 4, 20, CURSE, true} , {"Cleave", 6, 30, PHYS, false} }; 
    party.push_back(joker);

    Combatant skull{};
    skull.id = 1; skull.name = "Skull"; skull.texturePath = "assets/player_skull.png";
    skull.currentHp = skull.maxHp = 180;
    skull.currentSp = skull.maxSp = 35;
    skull.baseAttack = 18; skull.baseDefense = 12;
    skull.meleeWeapon = WEP_PIPE;
    skull.gunWeapon = GUN_SHOTGUN;
    skull.currentAmmo = skull.gunWeapon.magazineSize;
    skull.skills = { {"Lunge", 5, 25, PHYS, false}, {"Zio", 4, 25, ELEC, true} };
    skull.weaknesses = { WIND };
    party.push_back(skull);

    Combatant mona{};
    mona.id = 2; mona.name = "Mona"; mona.texturePath = "assets/player_mona.png";
    mona.currentHp = mona.maxHp = 100;
    mona.currentSp = mona.maxSp = 70;
    mona.baseAttack = 11; mona.baseDefense = 6;
    mona.meleeWeapon = WEP_SWORD;
    mona.gunWeapon = GUN_SLINGSHOT;
    mona.currentAmmo = mona.gunWeapon.magazineSize;
    mona.skills = { {"Garu", 4, 20, WIND, true}, {"Dia", 4, -30, ELEMENT_NONE, true} };
    mona.weaknesses = { ELEC };
    party.push_back(mona);

    Combatant noir{};
    noir.id = 3; noir.name = "Noir"; noir.texturePath = "assets/player_noir.png";
    noir.currentHp = noir.maxHp = 130;
    noir.currentSp = noir.maxSp = 55;
    noir.baseAttack = 15; noir.baseDefense = 10;
    noir.meleeWeapon = WEP_AXE;
    noir.gunWeapon = GUN_LAUNCHER;
    noir.currentAmmo = noir.gunWeapon.magazineSize;
    noir.skills = { {"Psi", 4, 35, PSI, true}, {"Mapsi", 8, 25, PSI, true} };
    noir.weaknesses = { NUKE };
    party.push_back(noir);

    return party;
}

// Helper to init default inventory
inline std::vector<Item> INITIAL_INVENTORY() {
    std::vector<Item> inv;
    inv.push_back(ITEM_MEDICINE);
    inv.push_back(ITEM_MEDICINE);
    inv.push_back(ITEM_MEDICINE);
    inv.push_back(ITEM_SNUFF_SOUL);
    return inv;
}

// Populate the bag with some better/worse items to test comparisons.
inline std::vector<Equipment> INITIAL_EQUIPMENTS() {
    std::vector<Equipment> bag;
    // Melee
    bag.push_back(MakeEquipment("Steel Pipe", EQUIP_MELEE, 12, 0, 0, PHYS, "Stronger than iron."));
    bag.push_back(MakeEquipment("Toy Hammer", EQUIP_MELEE, 2, 0, 0, PHYS, "Squeaky."));
    
    // Guns
    bag.push_back(MakeEquipment("Heavy Shotgun", EQUIP_GUN, 20, 0, 2, GUN, "High damage, low ammo."));
    bag.push_back(MakeEquipment("Pistol S", EQUIP_GUN, 5, 0, 16, GUN, "Weak but reliable."));

    // Armor
    bag.push_back(MakeEquipment("Kevlar Vest", EQUIP_ARMOR, 0, 12, 0, ELEMENT_NONE, "Solid protection."));
    bag.push_back(MakeEquipment("T-Shirt", EQUIP_ARMOR, 0, 1, 0, ELEMENT_NONE, "Better than nothing."));
    return bag;
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
            enemy.baseAttack = 10;
            enemy.baseDefense = 4;
            enemy.weaknesses = { GUN, ICE, CURSE };
            break;
        case 1: // Jack Frost (Nerfed for Level 2)
            enemy.name = "Jack Frost";
            enemy.maxHp = 90; enemy.currentHp = 90;
            enemy.baseAttack = 12;
            enemy.baseDefense = 5;
            enemy.weaknesses = { FIRE };
            break;
        case 2: // Agathion (Weak Elec)
            enemy.name = "Agathion";
            enemy.maxHp = 70; enemy.currentHp = 70;
            enemy.baseAttack = 11;
            enemy.baseDefense = 5;
            enemy.weaknesses = { ELEC };
            break;
        case 3: // Bicorn (Weak Wind)
            enemy.name = "Bicorn";
            enemy.maxHp = 80; enemy.currentHp = 80;
            enemy.baseAttack = 12;
            enemy.baseDefense = 6;
            enemy.weaknesses = { WIND };
            break;
        case 4: // Mandrake (Weak Fire)
            enemy.name = "Mandrake";
            enemy.maxHp = 85; enemy.currentHp = 85;
            enemy.baseAttack = 12;
            enemy.baseDefense = 6;
            enemy.weaknesses = { FIRE };
            break;
        case 10: // Kelpie (Weak Elec, Nerfed)
            enemy.name = "Kelpie";
            enemy.maxHp = 85; enemy.currentHp = 85;
            enemy.baseAttack = 11;
            enemy.baseDefense = 5;
            enemy.weaknesses = { ELEC };
            break;
        case 11: // Berith (Weak Ice, Nerfed)
            enemy.name = "Berith";
            enemy.maxHp = 110; enemy.currentHp = 110;
            enemy.baseAttack = 14;
            enemy.baseDefense = 7;
            enemy.weaknesses = { ICE };
            break;
        case 12: // Eligor (Weak Elec, Nerfed)
            enemy.name = "Eligor";
            enemy.maxHp = 100; enemy.currentHp = 100;
            enemy.baseAttack = 13;
            enemy.baseDefense = 6;
            enemy.weaknesses = { ELEC };
            break;
        case 13: // Hua Po (Weak Ice, Nerfed)
            enemy.name = "Hua Po";
            enemy.maxHp = 75; enemy.currentHp = 75;
            enemy.baseAttack = 10;
            enemy.baseDefense = 4;
            enemy.weaknesses = { ICE };
            break;
        case 99: // Boss placeholder
            enemy.name = "Shadow Boss";
            enemy.maxHp = 400; enemy.currentHp = 400;
            enemy.baseAttack = 30;
            enemy.baseDefense = 12;
            // No explicit weaknesses
            break;
        default: // Shadow
            enemy.name = "Shadow";
            enemy.maxHp = 30; enemy.currentHp = 30;
            enemy.baseAttack = 8;
            enemy.baseDefense = 3;
            break;
    }
    enemy.isAlive = true;
    enemy.isDown = false;
    return enemy;
}

// Add to your existing getEnemyData function or create a new wrapper
// IDs 0-9: Level 1 Pool, IDs 10-19: Level 2 Pool, ID 99: Boss
inline Combatant getRandomEnemyForLevel(int levelIndex) {
    int id = 0;
    if (levelIndex == 0) { // Level 1 (Easier)
        // Pool: Pixie, Agathion, Bicorn, Mandrake
        int pool[] = { 0, 2, 3, 4 };
        id = pool[GetRandomValue(0, 3)];
    } else if (levelIndex == 1) { // Level 2 (Harder)
        // Pool: Jack Frost, Kelpie, Berith, Eligor, Hua Po
        int pool[] = { 1, 10, 11, 12, 13 };
        id = pool[GetRandomValue(0, 4)];
    } else {
        id = 99; // Fallback to boss/placeholder for unknown levels
    }
    return getEnemyData(id);
}

// --- CHEST LOOT HELPERS ---
inline Item getRandomChestItem(int levelIndex) {
    if (levelIndex == 0) {
        Item pool[] = { ITEM_MEDICINE, ITEM_SNUFF_SOUL };
        int idx = GetRandomValue(0, 1);
        return pool[idx];
    } else {
        Item pool[] = { ITEM_MEDICINE, ITEM_SNUFF_SOUL, ITEM_REVIVAL_BEAD };
        int idx = GetRandomValue(0, 2);
        return pool[idx];
    }
}

inline Equipment getRandomChestEquipment(int levelIndex) {
    if (levelIndex == 0) {
        Equipment choices[] = {
            MakeEquipment("Rusty Knife", EQUIP_MELEE, 6, 0, 0, PHYS, "Old but sharp"),
            MakeEquipment("Light Pistol", EQUIP_GUN, 7, 0, 10, GUN, "Reliable sidearm"),
            MakeEquipment("Leather Jacket", EQUIP_ARMOR, 0, 6, 0, ELEMENT_NONE, "Basic protection")
        };
        int idx = GetRandomValue(0, 2);
        return choices[idx];
    } else {
        Equipment choices[] = {
            MakeEquipment("Steel Sword", EQUIP_MELEE, 12, 0, 0, PHYS, "Well-forged blade"),
            MakeEquipment("Heavy Revolver", EQUIP_GUN, 16, 0, 6, GUN, "Hard-hitting shots"),
            MakeEquipment("Reinforced Vest", EQUIP_ARMOR, 0, 12, 0, ELEMENT_NONE, "Solid defense")
        };
        int idx = GetRandomValue(0, 2);
        return choices[idx];
    }
}

#endif // GAME_DATA_H