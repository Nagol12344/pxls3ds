#include "config.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <vector>

#include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "log.h"
#include "utils/file_utils.h"

Config::Config() : timezoneOffset(0), language("en_US"), sslVerificationDisabled(false), fileLoggingEnabled(false) {}

void Config::load() {
	std::lock_guard<std::recursive_mutex> lock(mutex);

	struct stat st = {0};
	if (stat(CONFIG_DIR_PATH, &st) == -1) {
		mkdir(CONFIG_DIR_PATH, 0700);
	}

	loadSettings();
}

void Config::loadSettings() {
	std::string settingsPath = std::string(CONFIG_DIR_PATH) + "/settings.json";
	std::vector<char> buffer = Utils::File::readFile(settingsPath);

	if (!buffer.empty()) {
		rapidjson::Document doc;
		doc.Parse(buffer.data());

		if (!doc.HasParseError() && doc.IsObject()) {
			if (doc.HasMember("timezone_offset") && doc["timezone_offset"].IsInt()) {
				timezoneOffset = doc["timezone_offset"].GetInt();
			}
			if (doc.HasMember("language") && doc["language"].IsString()) {
				language = doc["language"].GetString();
			}
			if (doc.HasMember("ssl_verification_disabled") && doc["ssl_verification_disabled"].IsBool()) {
				sslVerificationDisabled = doc["ssl_verification_disabled"].GetBool();
			}
			if (doc.HasMember("file_logging") && doc["file_logging"].IsBool()) {
				fileLoggingEnabled = doc["file_logging"].GetBool();
			}
		} else {
			saveSettings();
		}
	} else {
		saveSettings();
	}

	Logger::setFileLoggingEnabled(fileLoggingEnabled);
}

void Config::saveSettings() {
	rapidjson::StringBuffer s;
	rapidjson::Writer<rapidjson::StringBuffer> writer(s);

	writer.StartObject();
	writer.Key("timezone_offset");
	writer.Int(timezoneOffset);
	writer.Key("language");
	writer.String(language.c_str());
	writer.Key("ssl_verification_disabled");
	writer.Bool(sslVerificationDisabled);
	writer.Key("file_logging");
	writer.Bool(fileLoggingEnabled);
	writer.EndObject();

	std::string settingsPath = std::string(CONFIG_DIR_PATH) + "/settings.json";
	Utils::File::writeFile(settingsPath, s.GetString());
}
