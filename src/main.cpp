#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include <random>

using namespace geode::prelude;

namespace rngutils
{
    template<typename T>
    T rng(T min, T max)
    {
        static_assert(std::is_arithmetic_v<T>, "T must be numeric");

        static thread_local std::mt19937_64 eng{std::random_device{}()};

        if constexpr (std::is_integral_v<T>)
        {
            std::uniform_int_distribution<T> dist(min, max);
            return dist(eng);
        }
        else
        {
            std::uniform_real_distribution<T> dist(min, max);
            return dist(eng);
        }
    }
}

class $modify(myPlayLayer, PlayLayer) {
	struct Fields {
		std::vector<std::string> m_phrases;
	};
	
	void destroyPlayer(PlayerObject* player, GameObject* object) {
		// no repeat messages
        static size_t lastPhraseIndex = std::numeric_limits<size_t>::max();
        if (!m_fields->m_phrases.empty() && object != m_anticheatSpike) {
            size_t phraseIndex;
            if (m_fields->m_phrases.size() == 1) {
                phraseIndex = 0;
            } else {
                do {
                    phraseIndex = rngutils::rng(size_t(0), m_fields->m_phrases.size() - 1);
                } while (phraseIndex == lastPhraseIndex);
            }
            lastPhraseIndex = phraseIndex;
            
            Notification::create(m_fields->m_phrases[phraseIndex], CCSprite::createWithSpriteFrameName("GJ_deleteIcon_001.png"))->show();
        }
        destroyPlayer(player, object);
	}

    static CCScene* scene(GJGameLevel* level, bool useReplay, bool dontCreateObjects) {
        CCScene* ret = PlayLayer::scene(level, useReplay, dontCreateObjects);
        static_cast<myPlayLayer*>(PlayLayer::get())->loadMessages();
        return ret;
    }

	void loadMessages() {
		std::ifstream infile(Mod::get()->getConfigDir() / "deathMessages.txt");
		if (!infile.is_open()) return;

		std::string line;
		while (std::getline(infile, line)) {
			size_t firstNonSpace = line.find_first_not_of(" \t");
			if (firstNonSpace == std::string::npos) continue;
			line = line.substr(firstNonSpace);

			if (line.size() >= 2 && line[0] == '#' && line[1] == ' ') continue;
			m_fields->m_phrases.push_back(line);
		}
	}
};

$on_mod(Loaded) {
    Mod* mod = Mod::get();
    auto resourceTxtLocation = mod->getResourcesDir() / "deathMessagesTemplate.txt";
    auto configTxtLocation = mod->getConfigDir() / "deathMessages.txt";

    if (!std::filesystem::exists(configTxtLocation)) {
        std::filesystem::copy_file(resourceTxtLocation, configTxtLocation);
    }    
};