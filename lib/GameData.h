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
    arsene.speed = 14;
    arsene.skills = { {"Eiha", 4, 20, CURSE, true}, {"Cleave", 6, 25, PHYS, false} };
    arsene.weaknesses = { ICE, BLESS };
    list.push_back(arsene);

    // 2. Pixie (Healer) - Low Stats, Good Skills, Weak to Gun/Ice
    Persona pixie;
    pixie.name = "Pixie";
    pixie.baseAttack = 8; 
    pixie.baseDefense = 4; 
    pixie.speed = 10;
    pixie.skills = { {"Zio", 4, 15, ELEC, true}, {"Dia", 3, -30, ELEMENT_NONE, true} };
    pixie.weaknesses = { GUN, ICE };
    list.push_back(pixie);

    // 3. Jack Frost (Mage) - Balanced, Weak to Fire
    Persona jack;
    jack.name = "Jack Frost";
    jack.baseAttack = 11; 
    jack.baseDefense = 8; 
    jack.speed = 9;
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
    joker.currentHp = joker.maxHp = 100;
    joker.currentSp = joker.maxSp = 50;
    joker.baseAttack = 10; joker.baseDefense = 5; joker.speed = 12; // Adjusted base stats
    joker.meleeWeapon = WEP_PARADISE_LOST;
    joker.gunWeapon = GUN_TKACHEV;
    joker.armor = ARM_TUXEDO;
    joker.currentAmmo = joker.gunWeapon.magazineSize;
    joker.skills = { {"Eiha", 4, 20, CURSE, true} , {"Cleave", 6, 30, PHYS, false} }; 
    party.push_back(joker);

    Combatant skull{};
    skull.id = 1; skull.name = "Skull"; skull.texturePath = "assets/player_skull.png";
    skull.currentHp = skull.maxHp = 150;
    skull.currentSp = skull.maxSp = 30;
    skull.baseAttack = 15; skull.baseDefense = 10; skull.speed = 8;
    skull.meleeWeapon = WEP_PIPE;
    skull.gunWeapon = GUN_SHOTGUN;
    skull.currentAmmo = skull.gunWeapon.magazineSize;
    skull.skills = { {"Lunge", 5, 25, PHYS, false}, {"Zio", 4, 25, ELEC, true} };
    skull.weaknesses = { WIND };
    party.push_back(skull);

    Combatant mona{};
    mona.id = 2; mona.name = "Mona"; mona.texturePath = "assets/player_mona.png";
    mona.currentHp = mona.maxHp = 80;
    mona.currentSp = mona.maxSp = 60;
    mona.baseAttack = 8; mona.baseDefense = 4; mona.speed = 15;
    mona.meleeWeapon = WEP_SWORD;
    mona.gunWeapon = GUN_SLINGSHOT;
    mona.currentAmmo = mona.gunWeapon.magazineSize;
    mona.skills = { {"Garu", 4, 20, WIND, true}, {"Dia", 4, -30, ELEMENT_NONE, true} };
    mona.weaknesses = { ELEC };
    party.push_back(mona);

    Combatant noir{};
    noir.id = 3; noir.name = "Noir"; noir.texturePath = "assets/player_noir.png";
    noir.currentHp = noir.maxHp = 110;
    noir.currentSp = noir.maxSp = 45;
    noir.baseAttack = 12; noir.baseDefense = 8; noir.speed = 10;
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
            enemy.weaknesses = { GUN, ICE, CURSE };
            break;
        case 1: // Jack Frost
            enemy.name = "Jack Frost";
            enemy.maxHp = 120; enemy.currentHp = 120;
            enemy.baseAttack = 15;
            enemy.weaknesses = { FIRE };
            break;
        default: // Shadow
            enemy.name = "Shadow";
            enemy.maxHp = 30; enemy.currentHp = 30;
            enemy.baseAttack = 8;
            break;
    }
    return enemy;
}

#endif // GAME_DATA_H