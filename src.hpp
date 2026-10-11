#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <set>
#include <sstream>
#include <algorithm>
#include <cstring>

class BasicException {
protected:
    const char *message;

public:
    explicit BasicException(const char *_message) : message(_message) {
    }

    virtual const char *what() const {
        return message;
    }
};

class ArgumentException: public BasicException {
public:
    explicit ArgumentException(const char *_message) : BasicException(_message) {
    }
};

class IteratorException: public BasicException {
public:
    explicit IteratorException(const char *_message) : BasicException(_message) {
    }
};

struct Pokemon {
    char name[12];
    int id;
    std::vector<std::string> types;
    
    Pokemon() : id(0) {
        memset(name, 0, sizeof(name));
    }
    
    Pokemon(const char *_name, int _id, const char *_types) : id(_id) {
        strncpy(name, _name, 11);
        name[11] = '\0';
        
        std::string typesStr(_types);
        std::stringstream ss(typesStr);
        std::string type;
        
        while (std::getline(ss, type, '#')) {
            if (!type.empty()) {
                types.push_back(type);
            }
        }
    }
};

class Pokedex {
private:
    std::map<int, Pokemon> pokemonMap;
    std::string fileName;
    
    // Type effectiveness map: attacker -> defender -> multiplier
    std::map<std::string, std::map<std::string, float>> typeChart;
    
    void initializeTypeChart() {
        // Initialize all to 1.0 (normal damage)
        std::vector<std::string> allTypes = {"fire", "water", "grass", "electric", "ice", "ground", "flying"};
        for (const auto& attacker : allTypes) {
            for (const auto& defender : allTypes) {
                typeChart[attacker][defender] = 1.0f;
            }
        }
        
        // Fire is super effective against grass, ice
        typeChart["fire"]["grass"] = 2.0f;
        typeChart["fire"]["ice"] = 2.0f;
        
        // Fire is not very effective against water, fire
        typeChart["fire"]["water"] = 0.5f;
        typeChart["fire"]["fire"] = 0.5f;
        
        // Water is super effective against fire, ground, rock
        typeChart["water"]["fire"] = 2.0f;
        typeChart["water"]["ground"] = 2.0f;
        
        // Water is not very effective against water, grass, dragon
        typeChart["water"]["water"] = 0.5f;
        typeChart["water"]["grass"] = 0.5f;
        
        // Grass is super effective against water, ground, rock
        typeChart["grass"]["water"] = 2.0f;
        typeChart["grass"]["ground"] = 2.0f;
        
        // Grass is not very effective against fire, grass, poison, flying, bug, dragon, steel
        typeChart["grass"]["fire"] = 0.5f;
        typeChart["grass"]["grass"] = 0.5f;
        typeChart["grass"]["flying"] = 0.5f;
        
        // Electric is super effective against water, flying
        typeChart["electric"]["water"] = 2.0f;
        typeChart["electric"]["flying"] = 2.0f;
        
        // Electric is not very effective against electric, grass, dragon
        typeChart["electric"]["electric"] = 0.5f;
        typeChart["electric"]["grass"] = 0.5f;
        
        // Electric has no effect against ground
        typeChart["electric"]["ground"] = 0.0f;
        
        // Ice is super effective against grass, ground, flying, dragon
        typeChart["ice"]["grass"] = 2.0f;
        typeChart["ice"]["ground"] = 2.0f;
        typeChart["ice"]["flying"] = 2.0f;
        
        // Ice is not very effective against fire, water, ice, steel
        typeChart["ice"]["fire"] = 0.5f;
        typeChart["ice"]["water"] = 0.5f;
        typeChart["ice"]["ice"] = 0.5f;
        
        // Ground is super effective against fire, electric, poison, rock, steel
        typeChart["ground"]["fire"] = 2.0f;
        typeChart["ground"]["electric"] = 2.0f;
        
        // Ground is not very effective against grass, bug
        typeChart["ground"]["grass"] = 0.5f;
        
        // Ground has no effect against flying
        typeChart["ground"]["flying"] = 0.0f;
        
        // Flying is super effective against grass, fighting, bug
        typeChart["flying"]["grass"] = 2.0f;
        
        // Flying is not very effective against electric, rock, steel
        typeChart["flying"]["electric"] = 0.5f;
    }
    
    bool isValidName(const char *name) const {
        if (!name || strlen(name) == 0 || strlen(name) > 10) {
            return false;
        }
        
        for (size_t i = 0; i < strlen(name); i++) {
            if (!((name[i] >= 'a' && name[i] <= 'z') || (name[i] >= 'A' && name[i] <= 'Z'))) {
                return false;
            }
        }
        return true;
    }
    
    bool isValidType(const std::string &type) const {
        std::set<std::string> validTypes = {"fire", "water", "grass", "electric", "ice", "ground", "flying"};
        return validTypes.find(type) != validTypes.end();
    }
    
    bool isValidTypes(const char *types) const {
        if (!types || strlen(types) == 0) {
            return false;
        }
        
        std::string typesStr(types);
        std::stringstream ss(typesStr);
        std::string type;
        int count = 0;
        
        while (std::getline(ss, type, '#')) {
            if (!type.empty()) {
                count++;
                if (count > 7 || !isValidType(type)) {
                    return false;
                }
            }
        }
        
        return count >= 1 && count <= 7;
    }
    
    void loadFromFile() {
        std::ifstream file(fileName);
        if (!file.is_open()) {
            return;
        }
        
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            
            std::stringstream ss(line);
            std::string name, idStr, types;
            
            if (std::getline(ss, name, ',') && 
                std::getline(ss, idStr, ',') && 
                std::getline(ss, types)) {
                
                // Remove "name=" prefix
                if (name.substr(0, 5) == "name=") {
                    name = name.substr(5);
                }
                
                // Remove "id=" prefix
                if (idStr.substr(0, 3) == "id=") {
                    idStr = idStr.substr(3);
                }
                
                // Remove "types=" prefix
                if (types.substr(0, 6) == "types=") {
                    types = types.substr(6);
                }
                
                try {
                    int id = std::stoi(idStr);
                    Pokemon pm(name.c_str(), id, types.c_str());
                    pokemonMap[id] = pm;
                } catch (...) {
                    // Skip invalid lines
                }
            }
        }
        
        file.close();
    }
    
    void saveToFile() {
        std::ofstream file(fileName);
        if (!file.is_open()) {
            return;
        }
        
        for (const auto& pair : pokemonMap) {
            const Pokemon& pm = pair.second;
            file << "name=" << pm.name << ",id=" << pm.id << ",types=";
            
            for (size_t i = 0; i < pm.types.size(); i++) {
                if (i > 0) file << "#";
                file << pm.types[i];
            }
            file << std::endl;
        }
        
        file.close();
    }

public:
    explicit Pokedex(const char *_fileName) : fileName(_fileName) {
        initializeTypeChart();
        loadFromFile();
    }

    ~Pokedex() {
        saveToFile();
    }

    bool pokeAdd(const char *name, int id, const char *types) {
        // Check for argument errors first
        if (!isValidName(name)) {
            throw ArgumentException(("Argument Error: PM Name Invalid (" + std::string(name) + ")").c_str());
        }
        
        if (!isValidTypes(types)) {
            std::string typesStr(types);
            std::stringstream ss(typesStr);
            std::string type;
            
            while (std::getline(ss, type, '#')) {
                if (!type.empty() && !isValidType(type)) {
                    throw ArgumentException(("Argument Error: PM Type Invalid (" + type + ")").c_str());
                }
            }
        }
        
        if (id <= 0 || id > 1000000000) {
            throw ArgumentException("Argument Error: PM ID Invalid");
        }
        
        // Check if name already exists
        for (const auto& pair : pokemonMap) {
            if (strcmp(pair.second.name, name) == 0) {
                return false;
            }
        }
        
        // Check if id already exists
        if (pokemonMap.find(id) != pokemonMap.end()) {
            return false;
        }
        
        Pokemon pm(name, id, types);
        pokemonMap[id] = pm;
        return true;
    }

    bool pokeDel(int id) {
        auto it = pokemonMap.find(id);
        if (it != pokemonMap.end()) {
            pokemonMap.erase(it);
            return true;
        }
        return false;
    }

    std::string pokeFind(int id) const {
        auto it = pokemonMap.find(id);
        if (it != pokemonMap.end()) {
            return std::string(it->second.name);
        }
        return "None";
    }

    std::string typeFind(const char *types) const {
        if (!isValidTypes(types)) {
            std::string typesStr(types);
            std::stringstream ss(typesStr);
            std::string type;
            
            while (std::getline(ss, type, '#')) {
                if (!type.empty() && !isValidType(type)) {
                    throw ArgumentException(("Argument Error: PM Type Invalid (" + type + ")").c_str());
                }
            }
        }
        
        std::vector<std::string> searchTypes;
        std::string typesStr(types);
        std::stringstream ss(typesStr);
        std::string type;
        
        while (std::getline(ss, type, '#')) {
            if (!type.empty()) {
                searchTypes.push_back(type);
            }
        }
        
        std::vector<const Pokemon*> results;
        
        for (const auto& pair : pokemonMap) {
            const Pokemon& pm = pair.second;
            bool hasAllTypes = true;
            
            for (const auto& searchType : searchTypes) {
                bool found = false;
                for (const auto& pmType : pm.types) {
                    if (pmType == searchType) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    hasAllTypes = false;
                    break;
                }
            }
            
            if (hasAllTypes) {
                results.push_back(&pm);
            }
        }
        
        if (results.empty()) {
            return "None";
        }
        
        std::sort(results.begin(), results.end(), 
                 [](const Pokemon* a, const Pokemon* b) { return a->id < b->id; });
        
        std::string result = std::to_string(results.size());
        for (const auto& pm : results) {
            result += "\n" + std::string(pm->name);
        }
        
        return result;
    }

    float attack(const char *type, int id) const {
        if (!isValidType(type)) {
            throw ArgumentException(("Argument Error: PM Type Invalid (" + std::string(type) + ")").c_str());
        }
        
        auto it = pokemonMap.find(id);
        if (it == pokemonMap.end()) {
            return -1.0f;
        }
        
        const Pokemon& defender = it->second;
        float totalDamage = 1.0f;
        
        for (const auto& defenderType : defender.types) {
            auto attackerIt = typeChart.find(type);
            if (attackerIt != typeChart.end()) {
                auto defenderIt = attackerIt->second.find(defenderType);
                if (defenderIt != attackerIt->second.end()) {
                    totalDamage *= defenderIt->second;
                }
            }
        }
        
        return totalDamage;
    }

    int catchTry() const {
        if (pokemonMap.empty()) {
            return 0;
        }
        
        // Start with the Pokemon with smallest id
        auto it = pokemonMap.begin();
        const Pokemon* starter = &it->second;
        
        std::set<int> owned;
        std::set<int> canCatch;
        
        // Initially own the starter
        owned.insert(starter->id);
        
        bool newCatch = true;
        while (newCatch) {
            newCatch = false;
            
            // Try to catch all Pokemon with current owned ones
            for (const auto& pair : pokemonMap) {
                int targetId = pair.first;
                const Pokemon& target = pair.second;
                
                if (owned.find(targetId) != owned.end() || canCatch.find(targetId) != canCatch.end()) {
                    continue;
                }
                
                // Check if any owned Pokemon can catch this target
                for (int ownedId : owned) {
                    const Pokemon& attacker = pokemonMap.at(ownedId);
                    
                    // Try each type the attacker has
                    for (const auto& attackType : attacker.types) {
                        float damage = attack(attackType.c_str(), targetId);
                        if (damage >= 2.0f) {
                            canCatch.insert(targetId);
                            newCatch = true;
                            break;
                        }
                    }
                    if (canCatch.find(targetId) != canCatch.end()) {
                        break;
                    }
                }
            }
            
            // Add newly caught Pokemon to owned
            for (int newId : canCatch) {
                owned.insert(newId);
            }
        }
        
        return owned.size();
    }

    struct iterator {
    private:
        std::map<int, Pokemon>* container;
        std::map<int, Pokemon>::iterator it;
        
    public:
        iterator() : container(nullptr) {}
        
        iterator(std::map<int, Pokemon>* _container, std::map<int, Pokemon>::iterator _it) 
            : container(_container), it(_it) {}
        
        iterator &operator++() {
            if (container && it != container->end()) {
                ++it;
            }
            return *this;
        }
        
        iterator &operator--() {
            if (container) {
                if (it == container->begin()) {
                    throw IteratorException("Iterator Error: Decrement begin()");
                }
                --it;
            }
            return *this;
        }
        
        iterator operator++(int) {
            iterator temp = *this;
            ++(*this);
            return temp;
        }
        
        iterator operator--(int) {
            iterator temp = *this;
            --(*this);
            return temp;
        }
        
        iterator & operator = (const iterator &rhs) {
            container = rhs.container;
            it = rhs.it;
            return *this;
        }
        
        bool operator == (const iterator &rhs) const {
            return container == rhs.container && it == rhs.it;
        }
        
        bool operator != (const iterator &rhs) const {
            return !(*this == rhs);
        }
        
        Pokemon & operator*() const {
            if (container && it != container->end()) {
                return it->second;
            }
            throw IteratorException("Iterator Error: Invalid dereference");
        }
        
        Pokemon *operator->() const {
            if (container && it != container->end()) {
                return &(it->second);
            }
            throw IteratorException("Iterator Error: Invalid access");
        }
    };

    iterator begin() {
        return iterator(&pokemonMap, pokemonMap.begin());
    }

    iterator end() {
        return iterator(&pokemonMap, pokemonMap.end());
    }
};