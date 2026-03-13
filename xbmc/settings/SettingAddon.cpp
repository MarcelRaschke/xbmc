/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SettingAddon.h"

#include "addons/Addon.h"
#include "addons/addoninfo/AddonInfo.h"
#include "addons/addoninfo/AddonType.h"
#include "settings/lib/SettingsManager.h"
#include "utils/StringUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

#include <mutex>

CSettingAddon::CSettingAddon(const std::string &id, CSettingsManager *settingsManager /* = nullptr */)
  : CSettingString(id, settingsManager)
{ }

CSettingAddon::CSettingAddon(const std::string &id, int label, const std::string &value, CSettingsManager *settingsManager /* = nullptr */)
  : CSettingString(id, label, value, settingsManager)
{ }

CSettingAddon::CSettingAddon(const std::string &id, const CSettingAddon &setting)
  : CSettingString(id, setting)
{
  copyaddontype(setting);
}

SettingPtr CSettingAddon::Clone(const std::string &id) const
{
  return std::make_shared<CSettingAddon>(id, *this);
}

ADDON::AddonType CSettingAddon::GetAddonType() const
{
  if (m_addonTypes.empty())
    return ADDON::AddonType::UNKNOWN;
  return m_addonTypes.front();
}

void CSettingAddon::SetAddonType(ADDON::AddonType addonType)
{
  m_addonTypes = {addonType};
}

bool CSettingAddon::Deserialize(const TiXmlNode *node, bool update /* = false */)
{
  std::unique_lock lock(m_critical);

  if (!CSettingString::Deserialize(node, update))
    return false;

  if (m_control && (m_control->GetType() != "button" || m_control->GetFormat() != "addon"))
  {
    CLog::Log(LOGERROR, "CSettingAddon: invalid <control> of \"{}\"", m_id);
    return false;
  }

  bool ok = false;
  std::string strAddonType;
  const TiXmlNode* constraints = node->FirstChild("constraints");
  if (constraints)
  {
    // get the addon type(s) - supports comma-separated list
    if (XMLUtils::GetString(constraints, "addontype", strAddonType) && !strAddonType.empty())
    {
      m_addonTypes.clear();
      for (auto& typeStr : StringUtils::Split(strAddonType, ","))
      {
        const ADDON::AddonType type =
            ADDON::CAddonInfo::TranslateType(StringUtils::Trim(typeStr));
        if (type != ADDON::AddonType::UNKNOWN)
        {
          m_addonTypes.push_back(type);
          ok = true;
        }
      }
    }
  }

  if (!ok && !update)
  {
    CLog::Log(LOGERROR, "CSettingAddon: error reading the addontype value \"{}\" of \"{}\"",
              strAddonType, m_id);
    return false;
  }

  return true;
}

void CSettingAddon::copyaddontype(const CSettingAddon &setting)
{
  CSettingString::Copy(setting);

  std::unique_lock lock(m_critical);
  m_addonTypes = setting.m_addonTypes;
}
