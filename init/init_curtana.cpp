/*
 * Copyright (C) 2020 ATGDroid <bythedroid@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
 
 
/*
* This code is used to update specific properties based on their
* values from /system and /vendor. 
*
* Created by ATGDroid @xda for Redmi Note 9 Pro
*/

#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <unistd.h>
#include <android-base/logging.h>
#define _REALLY_INCLUDE_SYS__SYSTEM_PROPERTIES_H_
#include <sys/_system_properties.h>

static const std::string PROPERTY_LOADER = "property_loader: ";


namespace android {
namespace init {

struct one_str_struct {
    const std::string prop;
};

/* set which properties should be updated from /system */
const struct one_str_struct required_system_properties[] = {
	{ "ro.build.fingerprint" },
	{ "ro.build.description" },
	{ "ro.build.version.incremental" },
	{ "ro.build.version.release" },
	{ "ro.treble.enabled" },
	{ "ro.expect.recovery_id" },
	{ "ro.product.mod_device" },
	{ "ro.build.version.security_patch" },
	{ "ro.build.version.sdk" },
	{ "" },
};

/* set which properties should be updated from /vendor */
const struct one_str_struct required_vendor_properties[] = {
	{ "ro.vendor.build.fingerprint" },
	{ "ro.vendor.build.security_patch" },
	{ "ro.vendor.build.date" },
	{ "ro.vendor.build.date.utc" },
	{ "" },
};

void property_update(char const prop[], char const value[]) {
    prop_info *pi;
    
    pi = (prop_info*) __system_property_find(prop);
    if (pi)
        __system_property_update(pi, value, strlen(value));
    else
        __system_property_add(prop, strlen(prop), value, strlen(value));
}

void property_symlink(char const prop1[], char const prop2[]) {
    char value[PROP_VALUE_MAX];
    if (__system_property_get(prop1, value) <= 0)
    return;
    LOG(INFO) << PROPERTY_LOADER << "Symlinking property value " << prop1 << " to " << prop2;
    property_update(prop2, value);
}

void load_property_file_by_mount_point(const std::string mount_point, const struct one_str_struct *required_mount_point_properties) {
LOG(INFO) << PROPERTY_LOADER << "Loading properties from " << mount_point;
const std::string mnt = "/s", property_file = mnt + "/build.prop";
mkdir(mnt.c_str(), 755);
mount(mount_point.c_str(), mnt.c_str(), "ext4", MS_RDONLY, NULL);
  struct stat st;
  if(stat(property_file.c_str(), &st) == 0) {
  LOG(INFO) << PROPERTY_LOADER << "Found property file: " << mount_point << "/build.prop";
  std::ifstream file(property_file.c_str());
  if (file.is_open()) {
  std::string local, value;
  size_t pos;
  while (getline(file, local)) {
  const struct one_str_struct *required_properties = required_mount_point_properties;
  while (!required_properties->prop.empty()) {
      if (required_properties->prop[0] == local[0] && 
         ((pos = local.find_first_of("=")) != std::string::npos) &&
          local.substr(0, pos) == required_properties->prop) {
          value = local.substr(pos + 1);
          LOG(INFO) << PROPERTY_LOADER << "Overriding property: " << required_properties->prop << "=" << value;
          property_update(required_properties->prop.c_str(), value.c_str());
       }
       required_properties++;
     }
    }
   }
  }
    umount(mnt.c_str());
    rmdir(mnt.c_str());
}

void vendor_load_properties() {
load_property_file_by_mount_point("system", required_system_properties);
load_property_file_by_mount_point("vendor", required_vendor_properties);
property_symlink("ro.build.fingerprint", "ro.bootimage.build.fingerprint");
}
}  // namespace init
}  // namespace android

