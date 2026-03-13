/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "addons/IAddon.h"
#include "settings/lib/Setting.h"

#include <vector>

namespace ADDON
{
enum class AddonType;
}
class TiXmlNode;

class CSettingAddon : public CSettingString
{
public:
  CSettingAddon(const std::string &id, CSettingsManager *settingsManager = nullptr);
  CSettingAddon(const std::string &id, int label, const std::string &value, CSettingsManager *settingsManager = nullptr);
  CSettingAddon(const std::string &id, const CSettingAddon &setting);
  ~CSettingAddon() override = default;

  SettingPtr Clone(const std::string &id) const override;

  bool Deserialize(const TiXmlNode *node, bool update = false) override;

  ADDON::AddonType GetAddonType() const;
  void SetAddonType(ADDON::AddonType addonType);
  const std::vector<ADDON::AddonType>& GetAddonTypes() const { return m_addonTypes; }
  void SetAddonTypes(std::vector<ADDON::AddonType> addonTypes) { m_addonTypes = std::move(addonTypes); }

private:
  void copyaddontype(const CSettingAddon &setting);

  std::vector<ADDON::AddonType> m_addonTypes;
};
