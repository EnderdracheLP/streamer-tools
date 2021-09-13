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

Transform* parent;

enum class SettingsState {
    NotInit, Updating, Ready
};

SettingsState state = SettingsState::NotInit;

//bool SettingsInit = false;

//bool NotUpdating = true;

// Defined here, and defined as extern in STmanager.hpp
GameObject* Decimals;
GameObject* DEnergy;
GameObject* D_MP_Code;
GameObject* A_MP_Code;
GameObject* A_Update;

void MakeConfigUI(bool UpdateOnly) {
    // Prevents a crash from the client trying to set the config before the config ViewController has first activated
    // This will update the Config Values while the config is open
    if (UpdateOnly && state != SettingsState::Ready) return;
    else if (UpdateOnly) {
        state = SettingsState::Updating;
        Decimals->GetComponent<IncrementSetting*>()->CurrentValue = (float)getModConfig().DecimalsForNumbers.GetValue();
        Decimals->GetComponent<IncrementSetting*>()->UpdateValue();
        DEnergy->GetComponent<Toggle*>()->set_isOn(getModConfig().DontEnergy.GetValue());
        D_MP_Code->GetComponent<Toggle*>()->set_isOn(getModConfig().DontMpCode.GetValue());
        A_MP_Code->GetComponent<Toggle*>()->set_isOn(getModConfig().AlwaysMpCode.GetValue());
        A_Update->GetComponent<Toggle*>()->set_isOn(getModConfig().AlwaysUpdate.GetValue());
        state = SettingsState::Ready;
    }
    // Creates the Config UI
    else {
        // Decimals for numbers
        getModConfig().DecimalsForNumbers.AddChangeEvent(
            [&](int value) {
                getModConfig().LastChanged.SetValue(static_cast<int>(time(0)));
                configFetched = false;
            }
        );

        Decimals = AddConfigValueIncrementInt(parent, getModConfig().DecimalsForNumbers, 1, 0, 20)->get_gameObject();
        BeatSaberUI::AddHoverHint(Decimals, "How many decimal places to show");


        //Decimals = STAddConfigValueIncrementInt(parent, getModConfig().DecimalsForNumbers, 1, 0, 20)->get_gameObject();
        //BeatSaberUI::AddHoverHint(Decimals, "How many decimal places to show");

        // DontEnergy
        getModConfig().DontEnergy.AddChangeEvent(
            [&](bool value) {
                getModConfig().LastChanged.SetValue(static_cast<int>(time(0)));
                configFetched = false;
            }
        );

        DEnergy = AddConfigValueToggle(parent, getModConfig().DontEnergy)->get_gameObject();
        BeatSaberUI::AddHoverHint(DEnergy, "Dont show energy bar");

        // DontMpCode
        getModConfig().DontMpCode.AddChangeEvent(
            [&](bool value) {
                getModConfig().LastChanged.SetValue(static_cast<int>(time(0)));
                configFetched = false;
            }
        );

        D_MP_Code = AddConfigValueToggle(parent, getModConfig().DontMpCode)->get_gameObject();
        BeatSaberUI::AddHoverHint(D_MP_Code, "Don't show multiplayer code");
        
        // AlwaysMpCode
        getModConfig().AlwaysMpCode.AddChangeEvent(
            [&](bool value) {
                getModConfig().LastChanged.SetValue(static_cast<int>(time(0)));
                configFetched = false;
            }
        );

        A_MP_Code = AddConfigValueToggle(parent, getModConfig().AlwaysMpCode)->get_gameObject();
        BeatSaberUI::AddHoverHint(A_MP_Code, "Always show multiplayer code. default: if no shown in game as *****");
        
        // AlwaysUpdate
        getModConfig().AlwaysUpdate.AddChangeEvent(
            [&](bool value) {
                getModConfig().LastChanged.SetValue(static_cast<int>(time(0)));
                configFetched = false;
            }
        );

        A_Update = AddConfigValueToggle(parent, getModConfig().AlwaysUpdate)->get_gameObject();
        BeatSaberUI::AddHoverHint(A_Update, "Update the overlay on song select an not just on song start");
        state = SettingsState::Ready;
    }
}

//bool InSettings = true;

void StreamerTools::stSettingViewController::DidActivate(bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    //InSettings = true;
    if(firstActivation && state == SettingsState::NotInit) {
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