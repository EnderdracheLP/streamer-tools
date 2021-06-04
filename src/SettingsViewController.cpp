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

void StreamerTools::stSettingViewController::DidActivate(bool firstActivation, bool addedToHierarchy, bool screenSystemEnabling) {
    if(firstActivation) {
        get_gameObject()->AddComponent<Touchable*>();
        GameObject* container = BeatSaberUI::CreateScrollableSettingsContainer(get_transform());
        Transform* parent = container->get_transform();

        QuestUI::BeatSaberUI::CreateText(parent, "Client configuration");

        // DecimalsForNumbers
        QuestUI::BeatSaberUI::AddHoverHint(AddConfigValueIncrementInt(parent, getModConfig().DecimalsForNumbers, 1, 0, 20)->get_gameObject(), "How many decimal places to show");

        // DontEnergy
        QuestUI::BeatSaberUI::AddHoverHint(AddConfigValueToggle(parent, getModConfig().DontEnergy)->get_gameObject(), "Dont show energy bar");
        
        // DontMpCode
        QuestUI::BeatSaberUI::AddHoverHint(AddConfigValueToggle(parent, getModConfig().DontMpCode)->get_gameObject(), "Don't show multiplayer code");
        
        // AlwaysMpCode
        QuestUI::BeatSaberUI::AddHoverHint(AddConfigValueToggle(parent, getModConfig().AlwaysMpCode)->get_gameObject(), "Always show multiplayer code. default: if no shown in game as *****");
        
        // AlwaysUpdate
        QuestUI::BeatSaberUI::AddHoverHint(AddConfigValueToggle(parent, getModConfig().AlwaysUpdate)->get_gameObject(), "Update the overlay on song select an not just on song start");
    }
}
