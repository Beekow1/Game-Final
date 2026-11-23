#pragma once
#include <utility>
#include "Items.hpp"

struct GameManager
{
    private:
        GameManager()  {}
        ~GameManager() {}

        int floor_start_money = 0;
        int money = 0;
        int enemies_killed = 0;
        int quota_enemies = 0;
        int total_enemies = 0;
        int quota_unit_price = 5;
        std::array<int, ItemCount> items{};
        std::array<int, ItemCount> floor_start_items{}; // save between floors


    public:
        bool tutorial_done = false;

        void startFloor()
        {
            floor_start_money = money;
            floor_start_items = items;
            resetFloor();
        }

        void resetFloor()
        {
            money = floor_start_money; 
            items = floor_start_items;
            enemies_killed = 0;
        }

        static GameManager& Get()
        {
            static GameManager instance;
            return instance;
        }

        GameManager(const GameManager&) = delete;
        GameManager& operator=(const GameManager&) = delete;
        
        void addMoney(int x) { money += x; }
        int getMoney() { return money; }

        void setQuota(int total, float quota_frac)
        {
            total_enemies = total;
            quota_enemies = (int)(total_enemies * quota_frac);
        }

        bool quotaMet() const { return enemies_killed >= quota_enemies; }

        int quotaPrice() const 
        {
            int diff = quota_enemies - enemies_killed;
            return diff * quota_unit_price; 
        }

        bool spend(int price) // used by shop
        {
            if (money < price) return false; 
            money -= price; 
            return true; 
        }

        std::pair<int, int> getEnemiesKilledStats() { return {enemies_killed, quota_enemies};}
        void enemyKilled() { enemies_killed++; }
        void enemyKilled(int mon) 
        { 
            enemies_killed++; 
            addMoney(mon);
        }

        // item functions
        void addItem(ItemId id) {
            items[size_t(id)]++; 
        }
        int itemCount(ItemId id) const {
            return items[size_t(id)];
        }
};