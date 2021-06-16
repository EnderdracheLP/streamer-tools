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
#else
DEFINE_TYPE(StreamerTools::stSettingViewController);
#endif

Transform* parent;

bool SettingsInit = false;

// Defined here, and defined as extern in STmanager.hpp
GameObject* Decimals;
GameObject* DEnergy;
GameObject* D_MP_Code;
GameObject* A_MP_Code;
GameObject* A_Update;

void MakeConfigUI(bool Update) {
    // Prevents a crash from the client trying to set the config before the config ViewController has first activated
    // This will update the Config Values while the config is open
    if (Update && !SettingsInit) return;
    else if (Update) {
        Decimals->GetComponent<IncrementSetting*>()->CurrentValue = (float)getModConfig().DecimalsForNumbers.GetValue();
        Decimals->GetComponent<IncrementSetting*>()->UpdateValue();
        DEnergy->GetComponent<Toggle*>()->set_isOn(getModConfig().DontEnergy.GetValue());
        D_MP_Code->GetComponent<Toggle*>()->set_isOn(getModConfig().DontMpCode.GetValue());
        A_MP_Code->GetComponent<Toggle*>()->set_isOn(getModConfig().AlwaysMpCode.GetValue());
        A_Update->GetComponent<Toggle*>()->set_isOn(getModConfig().AlwaysUpdate.GetValue());
    }
    // Creates the Config UI
    else {
        Decimals = AddConfigValueIncrementInt(parent, getModConfig().DecimalsForNumbers, 1, 0, 20)->get_gameObject();
        BeatSaberUI::AddHoverHint(Decimals, "How many decimal places to show");

        // DontEnergy
        DEnergy = AddConfigValueToggle(parent, getModConfig().DontEnergy)->get_gameObject();
        BeatSaberUI::AddHoverHint(DEnergy, "Dont show energy bar");

        // DontMpCode
        D_MP_Code = AddConfigValueToggle(parent, getModConfig().DontMpCode)->get_gameObject();
        BeatSaberUI::AddHoverHint(D_MP_Code, "Don't show multiplayer code");

        // AlwaysMpCode
        A_MP_Code = AddConfigValueToggle(parent, getModConfig().AlwaysMpCode)->get_gameObject();
        BeatSaberUI::AddHoverHint(A_MP_Code, "Always show multiplayer code. default: if no shown in game as *****");

        // AlwaysUpdate
        A_Update = AddConfigValueToggle(parent, getModConfig().AlwaysUpdate)->get_gameObject();
        BeatSaberUI::AddHoverHint(A_Update, "Update the overlay on song select an not just on song start");
        SettingsInit = true;
    }
}

void StreamerTools::stSettingViewController::DidActivate(bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    if(firstActivation) {
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
//    UnityEngine::GameObject::Destroy(container);
//}