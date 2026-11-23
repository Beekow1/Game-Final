#pragma once
#include <array> 
#include <cstddef> 
#include <cstdint>

// If adding items: Match item-icons.png from left to right per tile (enum value = icon frame)
enum class ItemId: uint8_t {MaxHP = 0, Dash, Attack, Jump, Coin, Radioactive, StyleDecay, Random, Count};
constexpr size_t ItemCount = size_t(ItemId::Count);

struct ItemDef {
    char const *name; 
    int price; 
    float shop_weight; // probability
};

inline constexpr std::array<ItemDef, ItemCount> ItemDefs = {{
    {"VITALITY",     15, 1.0f },
	{"EXTRA DASH",   25, 1.0f },
	{"SHARP CLAWS",  20, 1.0f },
	{"DOUBLE JUMP",  25, 1.0f },
	{"LUCKY COIN",   15, 1.0f },
	{"RADIOACTIVE",  40, 0.25f}, // rare
	{"STYLE KEEPER", 15, 1.0f },
	{"MYSTERY",      10, 1.0f },
}};

inline ItemDef const &itemDef(ItemId id) { return ItemDefs[size_t(id)]; }