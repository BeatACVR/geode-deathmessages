#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include <random>
#include <cmath>

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
    struct Phrase {
        std::string text;
        std::optional<int> percentConstraint;
    };

	struct Fields {
		std::vector<Phrase> m_phrases;
        geode::Ref<Notification> m_notification;
	};

    static std::optional<int> extractPercentSuffix(std::string& line) {
        size_t bsPos = line.rfind('\\');
        if (bsPos == std::string::npos) return std::nullopt;

        std::string suffix = line.substr(bsPos + 1);
        if (suffix.empty()) return std::nullopt;
        if (suffix.find('.') != std::string::npos) return std::nullopt;
        if (suffix[0] == '-') return std::nullopt;
        if (!std::all_of(suffix.begin(), suffix.end(), ::isdigit)) return std::nullopt;

        int value = utils::numFromString<int>(suffix).unwrapOr(-2);

        if (value < 0 || value > 100) return std::nullopt;
        size_t stripPos = bsPos;
        while (stripPos > 0 && line[stripPos - 1] == ' ') --stripPos;
        line = line.substr(0, stripPos);

        return value;
    }

	void destroyPlayer(PlayerObject* player, GameObject* object) {
        if (m_fields->m_notification) m_fields->m_notification->cancel();

        if (!m_fields->m_phrases.empty() && object != m_anticheatSpike) {
            int currentPercent = static_cast<int>(std::floor(getCurrentPercent()));
            bool isPlatformer = player->m_isPlatformer;

            std::vector<size_t> eligible;
            eligible.reserve(m_fields->m_phrases.size());
            for (size_t i = 0; i < m_fields->m_phrases.size(); ++i) {
                const Phrase& p = m_fields->m_phrases[i];
                if (p.percentConstraint.has_value()) {
                    if (isPlatformer) continue;
                    if (*p.percentConstraint != currentPercent) continue;
                }
                eligible.push_back(i);
            }

            if (!eligible.empty()) {
                static size_t lastPhraseIndex = std::numeric_limits<size_t>::max();

                size_t chosenIdx;
                if (eligible.size() == 1) {
                    chosenIdx = eligible[0];
                } else {
                    size_t pick;
                    do {
                        pick = rngutils::rng(size_t(0), eligible.size() - 1);
                    } while (eligible[pick] == lastPhraseIndex);
                    chosenIdx = eligible[pick];
                }
                lastPhraseIndex = chosenIdx;

                m_fields->m_notification = Notification::create(
                    m_fields->m_phrases[chosenIdx].text,
                    CCSprite::createWithSpriteFrameName("GJ_deleteIcon_001.png")
                );
                m_fields->m_notification->show();
            }
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

            Phrase p;
            p.percentConstraint = extractPercentSuffix(line);
            p.text = line;
            m_fields->m_phrases.push_back(std::move(p));
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