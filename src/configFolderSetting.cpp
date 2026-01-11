#include <Geode/Geode.hpp>

// stolen from palettesaver.cpp in my mega hack themes mod
using namespace geode::prelude;

class FolderButtonSettingV3 : public SettingV3 {
public:
    static Result<std::shared_ptr<SettingV3>> parse(std::string const& key, std::string const& modID, matjson::Value const& json) {
        auto res = std::make_shared<FolderButtonSettingV3>();
        auto root = checkJson(json, "FolderButtonSettingV3");

        res->init(key, modID, root);
        res->parseNameAndDescription(root);
        res->parseEnableIf(root);
        
        root.checkUnknownKeys();
        return root.ok(std::static_pointer_cast<SettingV3>(res));
    }

    bool load(matjson::Value const& json) override {
        return true;
    }
    bool save(matjson::Value& json) const override {
        return true;
    }

    bool isDefaultValue() const override {
        return true;
    }
    void reset() override {}

    SettingNodeV3* createNode(float width) override;
};

class FolderButtonSettingNodeV3 : public SettingNodeV3 {
protected:
    CircleButtonSprite* m_folderButtonSprite;
    CCMenuItemSpriteExtra* m_folderButton;

    bool init(std::shared_ptr<FolderButtonSettingV3> setting, float width) {
        if (!SettingNodeV3::init(setting, width))
            return false;
        
        m_folderButtonSprite = CircleButtonSprite::createWithSpriteFrameName("folderIcon_001.png", 1.1f, CircleBaseColor::Green);
        m_folderButtonSprite->setScale(.7f);
        m_folderButton = CCMenuItemSpriteExtra::create(
            m_folderButtonSprite, this, menu_selector(FolderButtonSettingNodeV3::onFolderButton)
        );

        this->getButtonMenu()->addChild(m_folderButton);
        this->getButtonMenu()->updateLayout();
        this->getButtonMenu()->setContentWidth(40);
        this->getButtonMenu()->setLayout(RowLayout::create());

        this->updateState(nullptr);
        
        return true;
    }

    void onFolderButton(CCObject*) {
        file::openFolder(Mod::get()->getConfigDir());
    }
    
    // Both of these can just be no-ops, since they make no sense for our 
    // setting as it's just a button
    void onCommit() override {}
    void onResetToDefault() override {}

public:
    static FolderButtonSettingNodeV3* create(std::shared_ptr<FolderButtonSettingV3> setting, float width) {
        auto ret = new FolderButtonSettingNodeV3();
        if (ret->init(setting, width)) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }

    bool hasUncommittedChanges() const override {
        return false;
    }
    bool hasNonDefaultValue() const override {
        return false;
    }

    std::shared_ptr<FolderButtonSettingV3> getSetting() const {
        return std::static_pointer_cast<FolderButtonSettingV3>(SettingNodeV3::getSetting());
    }
};

SettingNodeV3* FolderButtonSettingV3::createNode(float width) {
    return FolderButtonSettingNodeV3::create(
        std::static_pointer_cast<FolderButtonSettingV3>(shared_from_this()),
        width
    );
}

$execute {
    (void)Mod::get()->registerCustomSettingType("folder-button", &FolderButtonSettingV3::parse);
}