#include "STmanager.hpp"
#include "SettingsViewController.hpp"
#include "Config.hpp"

#include "UnityEngine/RectOffset.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/Vector2.hpp"
#include "UnityEngine/UI/Image.hpp"
#include "UnityEngine/UI/Toggle.hpp"
#include "UnityEngine/UI/Toggle_ToggleEvent.hpp"
#include "UnityEngine/UI/LayoutElement.hpp"
#include "UnityEngine/Events/UnityAction.hpp"
#include "UnityEngine/Events/UnityAction_1.hpp"
#include "HMUI/ScrollView.hpp"
#include "HMUI/ModalView.hpp"
#include "HMUI/Touchable.hpp"
#include "UnityEngine/Resources.hpp"
#include "GlobalNamespace/ColorPickerButtonController.hpp"
#include "GlobalNamespace/ColorChangeUIEventType.hpp"
#include "System/Action_2.hpp"

#include "questui/shared/QuestUI.hpp"
#include "questui/shared/BeatSaberUI.hpp"

using namespace QuestUI;
using namespace UnityEngine;
using namespace UnityEngine::UI;
using namespace UnityEngine::Events;
using namespace HMUI;

#ifndef REGISTER_FUNCTION
DEFINE_TYPE(StreamerTools, stSettingViewController);
#elif defined(DEFINE_TYPE)
DEFINE_TYPE(StreamerTools::stSettingViewController);
#elif defined(DEFINE_CLASS)
DEFINE_CLASS(StreamerTools::stSettingViewController);
#endif

bool configFetched = false;

// Using these functions instead of the ones builtin to config-utils as we need to add extra code
inline ::UnityEngine::UI::Toggle* STAddConfigValueToggle(::UnityEngine::Transform* parent, ConfigUtils::ConfigValue<bool>& configValue) {
    auto object = ::QuestUI::BeatSaberUI::CreateToggle(parent, configValue.GetName(), configValue.GetValue(),
        [&configValue](bool value) {
            configValue.SetValue(value);
            getModConfig().LastChanged.SetValue(static_cast<int>(time(0)));
            configFetched = false;
        }
    );
    if (!configValue.GetHoverHint().empty())
        ::QuestUI::BeatSaberUI::AddHoverHint(object->get_gameObject(), configValue.GetHoverHint());
    return object;
}

inline ::QuestUI::IncrementSetting* STAddConfigValueIncrementInt(::UnityEngine::Transform* parent, ConfigUtils::ConfigValue<int>& configValue, int increment, int min, int max) {
    auto object = ::QuestUI::BeatSaberUI::CreateIncrementSetting(parent, configValue.GetName(), 0, increment, configValue.GetValue(), min, max,
        [&configValue](float value) {
            configValue.SetValue((int)value);
            getModConfig().LastChanged.SetValue(static_cast<int>(time(0)));
            configFetched = false;
        }
    );
    if (!configValue.GetHoverHint().empty())
        ::QuestUI::BeatSaberUI::AddHoverHint(object->get_gameObject(), configValue.GetHoverHint());
    return object;
}

inline ::QuestUI::IncrementSetting* STAddConfigValueIncrementFloat(::UnityEngine::Transform* parent, ConfigUtils::ConfigValue<float>& configValue, int decimals, float increment, float min, float max) {
    auto object = ::QuestUI::BeatSaberUI::CreateIncrementSetting(parent, configValue.GetName(), decimals, increment, configValue.GetValue(), min, max,
        [&configValue](float value) {
            configValue.SetValue(value);
            getModConfig().LastChanged.SetValue(static_cast<int>(time(0)));
            configFetched = false;
        }
    );
    if (!configValue.GetHoverHint().empty())
        ::QuestUI::BeatSaberUI::AddHoverHint(object->get_gameObject(), configValue.GetHoverHint());
    return object;
}

Transform* parent;

bool SettingsInit = false;

// Defined here, and defined as extern in STmanager.hpp
GameObject* Decimals;
GameObject* DEnergy;
GameObject* D_MP_Code;
GameObject* A_MP_Code;
GameObject* A_Update;

void MakeConfigUI(bool UpdateOnly) {
    // Prevents a crash from the client trying to set the config before the config ViewController has first activated
    // This will update the Config Values while the config is open
    if (UpdateOnly && !SettingsInit) return;
    else if (UpdateOnly) {
        Decimals->GetComponent<IncrementSetting*>()->CurrentValue = (float)getModConfig().DecimalsForNumbers.GetValue();
        Decimals->GetComponent<IncrementSetting*>()->UpdateValue();
        DEnergy->GetComponent<Toggle*>()->set_isOn(getModConfig().DontEnergy.GetValue());
        D_MP_Code->GetComponent<Toggle*>()->set_isOn(getModConfig().DontMpCode.GetValue());
        A_MP_Code->GetComponent<Toggle*>()->set_isOn(getModConfig().AlwaysMpCode.GetValue());
        A_Update->GetComponent<Toggle*>()->set_isOn(getModConfig().AlwaysUpdate.GetValue());
    }
    // Creates the Config UI
    else {
        Decimals = STAddConfigValueIncrementInt(parent, getModConfig().DecimalsForNumbers, 1, 0, 20)->get_gameObject();
        BeatSaberUI::AddHoverHint(Decimals, "How many decimal places to show");

        // DontEnergy
        DEnergy = STAddConfigValueToggle(parent, getModConfig().DontEnergy)->get_gameObject();
        BeatSaberUI::AddHoverHint(DEnergy, "Dont show energy bar");

        // DontMpCode
        D_MP_Code = STAddConfigValueToggle(parent, getModConfig().DontMpCode)->get_gameObject();
        BeatSaberUI::AddHoverHint(D_MP_Code, "Don't show multiplayer code");

        // AlwaysMpCode
        A_MP_Code = STAddConfigValueToggle(parent, getModConfig().AlwaysMpCode)->get_gameObject();
        BeatSaberUI::AddHoverHint(A_MP_Code, "Always show multiplayer code. default: if no shown in game as *****");

        // AlwaysUpdate
        A_Update = STAddConfigValueToggle(parent, getModConfig().AlwaysUpdate)->get_gameObject();
        BeatSaberUI::AddHoverHint(A_Update, "Update the overlay on song select an not just on song start");
        SettingsInit = true;
    }
}

//bool InSettings = true;

void StreamerTools::stSettingViewController::DidActivate(bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    //InSettings = true;
    if(firstActivation && !SettingsInit) {
        get_gameObject()->AddComponent<Touchable*>();
        GameObject* container = BeatSaberUI::CreateScrollableSettingsContainer(get_transform());
        parent = container->get_transform();
        QuestUI::BeatSaberUI::CreateText(parent, "Client configuration");
        MakeConfigUI(false);
    } else MakeConfigUI(true);
}

//void StreamerTools::stSettingViewController::DidDeactivate(bool removedFromHierarchy, bool systemScreenDisabling)
//{
//    InSettings = false;
//}