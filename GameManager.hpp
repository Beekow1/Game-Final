#pragma once
#include <utility>

struct GameManager
{
    private:
        GameManager()  {}
        ~GameManager() {}

        int money = 0;
        int enemies_killed = 0;
        int quota_enemies = 0;
        int total_enemies = 0;


    public:
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

        std::pair<int, int> getEnemiesKilledStats() { return {enemies_killed, quota_enemies};}
        void enemyKilled() { enemies_killed++; }
        void enemyKilled(int mon) 
        { 
            enemies_killed++; 
            addMoney(mon);
        }
};