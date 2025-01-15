
#include "RE/P/PlayerCharacter.h"
#include <array>
#include <format>
#include <memory>

// The singleton base class.
template <class T> class Singleton {
public:
  [[nodiscard]] static T *GetSingleton() {
    static T singleton;
    return std::addressof(singleton);
  }

  Singleton(const Singleton &) = delete;
  Singleton(Singleton &&) = delete;
  Singleton &operator=(const Singleton &) = delete;
  Singleton &operator=(Singleton &&) = delete;

protected:
  Singleton() = default;
};

static void AddPotionToInventory() {
  auto player = RE::PlayerCharacter::GetSingleton();
  if (!player) {
    SKSE::log::error("Player not found.");
    return;
  }

  auto potion = RE::TESForm::LookupByID(RE::FormID(0x0003eb2e));
  if (!potion || potion->GetFormType() != RE::FormType::AlchemyItem) {
    SKSE::log::error("Potion not found.");
    return;
  }

  auto potionEffect = static_cast<RE::AlchemyItem *>(potion)->effects[0];
  if (potionEffect) {
    potionEffect->effectItem.magnitude = 1000;
    SKSE::log::info("magnitude : {}", potionEffect->effectItem.magnitude);
    // RE::DebugNotification(xdd.c_str());
  }

  player->AddObjectToContainer(potion->As<RE::TESBoundObject>(), nullptr, 1,
                               nullptr);
  RE::DebugNotification("Potion added");
}

// static void AddArmorToInventory()
// {
// 	auto player = RE::PlayerCharacter::GetSingleton();
// 	if (!player)
// 	{
// 		SKSE::log::error("Player not found.");
// 		return;
// 	}

// 	auto armor = RE::TESForm::LookupByID(RE::FormID(0x000E35D7));
// 	if (!armor || armor->GetFormType() != RE::FormType::Armor)
// 	{
// 		SKSE::log::error("Armor not found.");
// 		return;
// 	}

// 	RE::TESEquipEvent

// 	// Add the armor to the player's inventory.
// 	player->AddObjectToContainer(armor->As<RE::TESBoundObject>(), nullptr,
// 1, nullptr); 	RE::DebugNotification("Armor added");
// }

static bool g_enable_enchant_removing = false;
static constexpr auto guild_master_armor_pieces =
    std::to_array({0x000E35D7ul, 0x000E35D6ul, 0x000E35D8ul, 0x000E35D9ul});

class OnEquipEventHandler final : public Singleton<OnEquipEventHandler>,
                                  public RE::BSTEventSink<RE::TESEquipEvent> {
  friend class Singleton<OnEquipEventHandler>;

public:
  using Event = RE::TESEquipEvent;
  using EventSource = RE::BSTEventSource<Event>;

  static void Register() {
    if (auto manager = RE::ScriptEventSourceHolder::GetSingleton()) {
      manager->AddEventSink(GetSingleton());
      SKSE::log::info("Successfully registered On Equip event handler.");
    } else {
      SKSE::log::error("Failed to register On Equip event handler.");
    }
  }

  RE::BSEventNotifyControl ProcessEvent(const Event *a_event,
                                        EventSource *a_eventSource) override {
    // if (!g_enable_enchant_removing) {
    return RE::BSEventNotifyControl::kContinue;
    // }

    (void *)a_eventSource;

    auto bla = (RE::TESEquipEvent *)a_event;
    if (!bla || !bla->actor ||
        bla->actor->IsNot(RE::FormType::ActorCharacter)) {
      SKSE::log::info("bail 1");
      return RE::BSEventNotifyControl::kContinue;
    }

    if (auto *form = RE::TESForm::LookupByID(bla->baseObject);
        !form || form->IsNot(RE::FormType::Armor)) {
      SKSE::log::info("bail 2");
      return RE::BSEventNotifyControl::kContinue;
    }

    auto refHandle = bla->actor->GetHandle();

    auto refr = refHandle.get();
    if (!refr || refr->IsNot(RE::FormType::ActorCharacter)) {
      SKSE::log::info("bail 3");
      return RE::BSEventNotifyControl::kContinue;
    }

    auto *actor = refr->As<RE::Actor>();
    auto *changes = actor->GetInventoryChanges();
    if (!changes || !changes->entryList) {
      SKSE::log::info("bail 4");
      return RE::BSEventNotifyControl::kContinue;
    }

    // auto inventory = RE::PlayerCharacter::GetSingleton()->GetInventory();

    for (auto &entry : *changes->entryList) {
      if (!entry->object) {
        SKSE::log::info("bad entry");
        continue;
      }

      SKSE::log::info("Entry name: {}", entry->GetDisplayName());

      bool need_patch = false;
      for (const auto piece : guild_master_armor_pieces) {
        if (entry->object->GetFormID() == piece) {
          SKSE::log::info("{:X}: need patch {}", entry->object->GetFormID(),
                          entry->GetDisplayName());
          need_patch = true;
          break;
        }
      }

      if (!need_patch) {
        SKSE::log::info("not needing patch, skipping! {:X} {}",
                        entry->object->GetFormID(), entry->GetDisplayName());
        continue;
      }

      if (!entry->extraLists) {
        SKSE::log::info("no extra list for {}", entry->GetDisplayName());
        continue;
      }

      // TODO: formEnchant handling (prefab items)

      for (auto &xList : *entry->extraLists) {
        auto xEnchantment = xList->GetByType<RE::ExtraEnchantment>();
        if (xEnchantment) {
          SKSE::log::info("removing!");
          xList->Remove(RE::ExtraDataType::kEnchantment, xEnchantment);
        } else {
          SKSE::log::info("no enchant");
        }

        auto xCharge = xList->GetByType<RE::ExtraCharge>();
        if (xCharge) {
          SKSE::log::info("removing charge!");
          xList->Remove(RE::ExtraDataType::kCharge, xCharge);
        } else {
          SKSE::log::info("no charge");
        }
      }
    }

    SKSE::log::info("bail 5");
    return RE::BSEventNotifyControl::kContinue;
  }

private:
  OnEquipEventHandler() = default;
};

class InputEventSink final : public Singleton<InputEventSink>,
                             public RE::BSTEventSink<RE::InputEvent *> {
  friend class Singleton<InputEventSink>;

public:
  using Event = RE::InputEvent *;
  using EventSource = RE::BSTEventSource<Event>;

  static void Register() {
    if (auto manager = RE::BSInputDeviceManager::GetSingleton()) {
      manager->AddEventSink(GetSingleton());
      SKSE::log::info("Successfully registered input event.");
    } else {
      SKSE::log::error("Failed to register input event.");
    }
  }

  RE::BSEventNotifyControl ProcessEvent(const Event *a_event,
                                        EventSource *a_eventSource) override {
    (void *)a_eventSource;

    if (!a_event || !RE::Main::GetSingleton()->gameActive) {
      return RE::BSEventNotifyControl::kContinue;
    }

    for (auto event = *a_event; event; event = event->next) {
      if (auto button = event->AsButtonEvent()) {
        if (button->IsDown()) {
          // auto xdd = std::format("Key Pressed : {}", button->GetIDCode());

          constexpr std::uint32_t keycode_k = 37;
          if (button->GetIDCode() == keycode_k) {
            AddPotionToInventory();
          }

          constexpr std::uint32_t keycode_h = 35;
          if (button->GetIDCode() == keycode_h) {
            g_enable_enchant_removing = !g_enable_enchant_removing;

            std::string xdd = std::format("g_enable_enchant_removing: {}",
                                          g_enable_enchant_removing);
            SKSE::log::info("{}", xdd);
            RE::DebugNotification(xdd.c_str());

            auto invMap = RE::PlayerCharacter::GetSingleton()->GetInventory();

            for (const auto piece : guild_master_armor_pieces) {
              auto player = RE::PlayerCharacter::GetSingleton();

              auto pieceForm = RE::TESForm::LookupByID(RE::FormID(piece))
                                   ->As<RE::TESObjectARMO>();

              if (pieceForm) {
                if (pieceForm->templateArmor) {
                  player->AddObjectToContainer(pieceForm->templateArmor,
                                               nullptr, 1, nullptr);
                } else {
                  RE::DebugNotification("no template armor");
                }
              } else {
                RE::DebugNotification("no piece armor");
              }

              // std::string dbgtxt = std::format("Adding {}",
              // RE::TESForm::LookupByID(RE::FormID(piece)));
              // RE::DebugNotification(dbgtxt.c_str());
            }
            return RE::BSEventNotifyControl::kContinue;

            for (auto &[k, v] : invMap) {
              if (v.second) {
                if (v.second->extraLists) {

                  for (const auto piece : guild_master_armor_pieces) {
                    if (k->GetFormID() == piece) {
                      SKSE::log::info("{} is {}", v.second->GetDisplayName(),
                                      v.second->IsEnchanted());

                      if (v.second->object) {
                        auto ench =
                            v.second->object->As<RE::TESEnchantableForm>();
                        if (ench && ench->formEnchanting) {
                          ench->formEnchanting = nullptr;
                          ench->amountofEnchantment = 0;
                        }
                      }

                      for (auto &xList : *v.second->extraLists) {
                        if (xList) {

                          for (auto &xList2 : *xList) {
                            SKSE::log::info("ExtraData: {}",
                                            (int)xList2.GetType());
                          }

                          auto xEnchantment =
                              xList->GetByType<RE::ExtraEnchantment>();

                          if (xEnchantment) {
                            SKSE::log::info("enchant removed");
                            xList->Remove(RE::ExtraDataType::kEnchantment,
                                          xEnchantment);
                          } else {
                            SKSE::log::info("no enchant");
                          }

                          auto xCharge = xList->GetByType<RE::ExtraCharge>();

                          if (xCharge) {
                            SKSE::log::info("charge removed");
                            xList->Remove(RE::ExtraDataType::kCharge, xCharge);
                          } else {
                            SKSE::log::info("no charge");
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }

    return RE::BSEventNotifyControl::kContinue;
  }

private:
  InputEventSink() = default;
};

void OnMessage(SKSE::MessagingInterface::Message *a_message) {
  switch (a_message->type) {
  case SKSE::MessagingInterface::kInputLoaded:
    InputEventSink::Register();
    break;
  case SKSE::MessagingInterface::kDataLoaded:
    OnEquipEventHandler::Register();
    break;
  default:
    break;
  }
}

SKSEPluginLoad(const SKSE::LoadInterface *a_skse) {
  SKSE::Init(a_skse);

  if (const auto intfc = SKSE::GetMessagingInterface())
    intfc->RegisterListener(OnMessage);

  return true;
}
